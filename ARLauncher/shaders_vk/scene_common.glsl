// Общие данные кадра для 3D-объектов (mesh.vert / mesh.frag)

layout(set = 1, binding = 0) uniform Frame {
    mat4 proj;
    mat4 cvFromEye;      // mat3: оси глаза -> оси камеры iPhone (OpenCV) — как у passthrough
    mat4 worldFromEye;   // mat3: поворот глаза в мире (SH и карта окружения — в мире)
    vec4 cam;            // fx/w, fy/h, cx/w, cy/h
    vec4 lightDir;       // к источнику, оси глаза
    vec4 lightColor;     // линейный RGB; w = сила тени
    vec4 sh[9];          // SH окружения (мир, линейный RGB)
    vec4 params;         // x флаги: 1 окклюзия, 2 LiDAR есть, 4 видео есть, 8 отражения, 16 карта окружения;
                         // y: 1 full range, 2 BT.709; z: мягкость окклюзии (м); w: непрозрачность
} f;

layout(push_constant) uniform Object {
    mat4 modelView;
    vec4 color;          // sRGB + альфа
    vec4 material;       // metallic, roughness, режим (0 объект, 1 тень), запас окклюзии (м)
} o;
