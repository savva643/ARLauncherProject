#include "VioTracker.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#ifdef LENSENGINE_WITH_OPENVINS
#include <opencv2/imgproc.hpp>
#include <unistd.h>
#include <sys/stat.h>

#include "core/VioManager.h"
#include "core/VioManagerOptions.h"
#include "state/State.h"
#include "types/IMU.h"
#include "utils/opencv_yaml_parse.h"
#include "utils/print.h"
#include "utils/quat_ops.h"
#include "utils/sensor_data.h"
#endif

namespace LensEngine {

#ifdef LENSENGINE_WITH_OPENVINS

namespace {

// Ширина изображения для VIO: 640 px — баланс точности и скорости (как EuRoC 752x480)
constexpr int kVioWidth = 640;

void writeFile(const std::string& path, const std::string& text)
{
    if (FILE* f = std::fopen(path.c_str(), "w")) {
        std::fwrite(text.data(), 1, text.size(), f);
        std::fclose(f);
    }
}

// Конфиг под iPhone: моно, 100 Гц IMU, онлайн-калибровка extrinsics/интринсиков/сдвига времени
std::string estimatorYaml()
{
    return R"(%YAML:1.0
verbosity: "WARNING"
use_fej: true
integration: "rk4"
use_stereo: false
max_cameras: 1
calib_cam_extrinsics: true
calib_cam_intrinsics: true
calib_cam_timeoffset: true
calib_imu_intrinsics: false
calib_imu_g_sensitivity: false
max_clones: 11
max_slam: 50
max_slam_in_update: 25
max_msckf_in_update: 40
dt_slam_delay: 1
gravity_mag: 9.80665
feat_rep_msckf: "GLOBAL_3D"
feat_rep_slam: "ANCHORED_MSCKF_INVERSE_DEPTH"
feat_rep_aruco: "ANCHORED_MSCKF_INVERSE_DEPTH"
try_zupt: true
zupt_chi2_multipler: 0
zupt_max_velocity: 0.1
zupt_noise_multiplier: 10
zupt_max_disparity: 0.5
zupt_only_at_beginning: false
init_window_time: 1.5
init_imu_thresh: 1.0
init_max_disparity: 2.0
init_max_features: 50
init_dyn_use: false
init_dyn_mle_opt_calib: false
init_dyn_mle_max_iter: 50
init_dyn_mle_max_time: 0.05
init_dyn_mle_max_threads: 4
init_dyn_num_pose: 6
init_dyn_min_deg: 10.0
init_dyn_inflation_ori: 10
init_dyn_inflation_vel: 100
init_dyn_inflation_bg: 10
init_dyn_inflation_ba: 100
init_dyn_min_rec_cond: 1e-12
init_dyn_bias_g: [ 0.0, 0.0, 0.0 ]
init_dyn_bias_a: [ 0.0, 0.0, 0.0 ]
record_timing_information: false
record_timing_filepath: "/tmp/traj_timing.txt"
save_total_state: false
filepath_est: "/tmp/ov_estimate.txt"
filepath_std: "/tmp/ov_estimate_std.txt"
filepath_gt: "/tmp/ov_groundtruth.txt"
use_klt: true
num_pts: 200
fast_threshold: 20
grid_x: 5
grid_y: 5
min_px_dist: 10
knn_ratio: 0.70
track_frequency: 31.0
downsample_cameras: false
num_opencv_threads: 2
histogram_method: "HISTOGRAM"
use_aruco: false
num_aruco: 1024
downsize_aruco: true
up_msckf_sigma_px: 1.5
up_msckf_chi2_multipler: 1
up_slam_sigma_px: 1.5
up_slam_chi2_multipler: 1
up_aruco_sigma_px: 1
up_aruco_chi2_multipler: 1
use_mask: false
relative_config_imu: "kalibr_imu_chain.yaml"
relative_config_imucam: "kalibr_imucam_chain.yaml"
)";
}

// Шумы IMU iPhone (консервативно, по Allan variance типичных телефонных MEMS)
std::string imuYaml()
{
    return R"(%YAML:1.0
imu0:
  T_i_b:
    - [1.0, 0.0, 0.0, 0.0]
    - [0.0, 1.0, 0.0, 0.0]
    - [0.0, 0.0, 1.0, 0.0]
    - [0.0, 0.0, 0.0, 1.0]
  accelerometer_noise_density: 2.0e-2
  accelerometer_random_walk: 2.0e-3
  gyroscope_noise_density: 2.0e-3
  gyroscope_random_walk: 4.0e-5
  rostopic: /imu0
  time_offset: 0.0
  update_rate: 100.0
  model: "kalibr"
  Tw:
    - [ 1.0, 0.0, 0.0 ]
    - [ 0.0, 1.0, 0.0 ]
    - [ 0.0, 0.0, 1.0 ]
  R_IMUtoGYRO:
    - [ 1.0, 0.0, 0.0 ]
    - [ 0.0, 1.0, 0.0 ]
    - [ 0.0, 0.0, 1.0 ]
  Ta:
    - [ 1.0, 0.0, 0.0 ]
    - [ 0.0, 1.0, 0.0 ]
    - [ 0.0, 0.0, 1.0 ]
  R_IMUtoACC:
    - [ 1.0, 0.0, 0.0 ]
    - [ 0.0, 1.0, 0.0 ]
    - [ 0.0, 0.0, 1.0 ]
  Tg:
    - [ 0.0, 0.0, 0.0 ]
    - [ 0.0, 0.0, 0.0 ]
    - [ 0.0, 0.0, 0.0 ]
)";
}

// Камера iPhone (OpenCV-оси) относительно осей IMU/устройства:
// x_cam = -y_dev, y_cam = -x_dev, z_cam = -z_dev (задняя камера смотрит в -z экрана).
std::string camYaml(int w, int h, double fx, double fy, double cx, double cy)
{
    std::ostringstream s;
    s << "%YAML:1.0\n"
      << "cam0:\n"
      << "  T_imu_cam:\n"
      << "    - [0.0, -1.0, 0.0, 0.0]\n"
      << "    - [-1.0, 0.0, 0.0, 0.0]\n"
      << "    - [0.0, 0.0, -1.0, 0.0]\n"
      << "    - [0.0, 0.0, 0.0, 1.0]\n"
      << "  cam_overlaps: []\n"
      << "  camera_model: pinhole\n"
      << "  distortion_coeffs: [0.0, 0.0, 0.0, 0.0]\n"
      << "  distortion_model: radtan\n"
      << "  intrinsics: [" << fx << ", " << fy << ", " << cx << ", " << cy << "]\n"
      << "  resolution: [" << w << ", " << h << "]\n"
      << "  rostopic: /cam0/image_raw\n"
      << "  timeshift_cam_imu: 0.0\n";
    return s.str();
}

} // namespace

struct VioTracker::Impl {
    std::shared_ptr<ov_msckf::VioManager> vio;
    int64_t t0 = 0;          // база времени (double секунды теряют точность на больших ns)
    bool haveT0 = false;
    int width = 0, height = 0;
    int64_t lastFedImu = 0;

    double toSec(int64_t tNs) const { return double(tNs - t0) * 1e-9; }

    bool create(int w, int h, float fx, float fy, float cx, float cy)
    {
        char dirTemplate[] = "/tmp/glaski_openvins_XXXXXX";
        char* dir = mkdtemp(dirTemplate);
        if (!dir) return false;
        const std::string base = std::string(dir) + "/";
        writeFile(base + "estimator_config.yaml", estimatorYaml());
        writeFile(base + "kalibr_imu_chain.yaml", imuYaml());
        writeFile(base + "kalibr_imucam_chain.yaml", camYaml(w, h, fx, fy, cx, cy));

        ov_core::Printer::setPrintLevel("WARNING");
        auto parser = std::make_shared<ov_core::YamlParser>(base + "estimator_config.yaml");
        ov_msckf::VioManagerOptions params;
        params.print_and_load(parser);
        params.use_multi_threading_subs = false;
        if (!parser->successful()) {
            std::cerr << "[VIO] OpenVINS config parse failed" << std::endl;
            return false;
        }
        vio = std::make_shared<ov_msckf::VioManager>(params);
        width = w;
        height = h;
        std::cout << "[VIO] OpenVINS ready: " << w << "x" << h << " fx=" << fx << std::endl;
        return true;
    }
};

bool VioTracker::available() { return true; }

#else

struct VioTracker::Impl {};
bool VioTracker::available() { return false; }

#endif // LENSENGINE_WITH_OPENVINS

VioTracker::VioTracker() : m_impl(std::make_unique<Impl>()) {}

VioTracker::~VioTracker()
{
    stop();
}

bool VioTracker::start()
{
    if (!available()) return false;
    if (m_running) return true;
    m_running = true;
    m_thread = std::thread(&VioTracker::worker, this);
    return true;
}

void VioTracker::stop()
{
    if (!m_running) return;
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
}

void VioTracker::addImu(int64_t tNs, const glm::dvec3& gyro, const glm::dvec3& accel)
{
    if (!m_running) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (tNs <= m_lastImuT) return;
        m_lastImuT = tNs;
        m_imu.push_back({tNs, gyro, accel});
        if (m_imu.size() > 2000) m_imu.pop_front();
    }
    m_cv.notify_one();
}

void VioTracker::addFrame(int64_t tNs, const uint8_t* y, int width, int height, int stride,
                          float fx, float fy, float cx, float cy)
{
    if (!m_running || !y || width <= 0 || height <= 0) return;
    auto f = std::make_unique<FrameItem>();
    f->tNs = tNs;
    f->width = width;
    f->height = height;
    f->fx = fx; f->fy = fy; f->cx = cx; f->cy = cy;
    f->gray.resize(size_t(width) * height);
    for (int r = 0; r < height; ++r) std::memcpy(f->gray.data() + size_t(r) * width, y + size_t(r) * stride, width);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_frame = std::move(f);   // старый кадр, если VIO не успел, выбрасываем
    }
    m_cv.notify_one();
}

void VioTracker::worker()
{
#ifdef LENSENGINE_WITH_OPENVINS
    Impl& s = *m_impl;
    std::deque<ImuItem> imu;
    while (m_running) {
        std::unique_ptr<FrameItem> frame;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait_for(lock, std::chrono::milliseconds(50),
                          [this] { return !m_running || !m_imu.empty() || m_frame; });
            if (!m_running) break;
            imu.insert(imu.end(), m_imu.begin(), m_imu.end());
            m_imu.clear();
            // Кадр можно отдавать, только когда IMU уже покрывает его время
            if (m_frame && m_lastImuT >= m_frame->tNs + 5'000'000) frame = std::move(m_frame);
        }

        if (!s.haveT0 && !imu.empty()) {
            s.t0 = imu.front().tNs;
            s.haveT0 = true;
        }

        if (frame && !s.vio) {
            const double scale = double(kVioWidth) / double(frame->width);
            const int w = kVioWidth;
            const int h = int(std::lround(frame->height * scale));
            if (!s.create(w, h, float(frame->fx * scale), float(frame->fy * scale),
                          float(frame->cx * scale), float(frame->cy * scale))) {
                m_running = false;
                break;
            }
        }

        if (s.vio) {
            for (const auto& m : imu) {
                if (m.tNs <= s.lastFedImu) continue;
                ov_core::ImuData d;
                d.timestamp = s.toSec(m.tNs);
                d.wm << m.gyro.x, m.gyro.y, m.gyro.z;
                d.am << m.accel.x, m.accel.y, m.accel.z;
                s.vio->feed_measurement_imu(d);
                s.lastFedImu = m.tNs;
            }
            imu.clear();
        } else if (imu.size() > 1000) {
            imu.erase(imu.begin(), imu.begin() + long(imu.size() - 1000));
        }

        if (!frame || !s.vio || !s.haveT0 || frame->tNs <= s.t0) continue;

        cv::Mat full(frame->height, frame->width, CV_8UC1, frame->gray.data());
        cv::Mat small;
        cv::resize(full, small, cv::Size(s.width, s.height), 0, 0, cv::INTER_AREA);

        ov_core::CameraData cam;
        cam.timestamp = s.toSec(frame->tNs);
        cam.sensor_ids.push_back(0);
        cam.images.push_back(small);
        cam.masks.push_back(cv::Mat::zeros(small.rows, small.cols, CV_8UC1));
        s.vio->feed_measurement_camera(cam);

        if (s.vio->initialized()) {
            if (!m_initialized.exchange(true)) std::cout << "[VIO] initialized — 6DoF tracking active" << std::endl;
            auto state = s.vio->get_state();
            const Eigen::Matrix3d R_GtoI = ov_core::quat_2_Rot(state->_imu->quat());
            const Eigen::Quaterniond q(R_GtoI.transpose());
            const Eigen::Vector3d p = state->_imu->pos();
            const Eigen::Vector3d v = state->_imu->vel();
            Pose pose;
            pose.tNs = s.t0 + int64_t(state->_timestamp * 1e9);
            pose.orientation = glm::normalize(glm::dquat(q.w(), q.x(), q.y(), q.z()));
            pose.position = glm::dvec3(p.x(), p.y(), p.z());
            pose.velocity = glm::dvec3(v.x(), v.y(), v.z());
            if (m_onPose) m_onPose(pose);
        }
    }
#endif
}

} // namespace LensEngine
