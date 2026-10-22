#!/usr/bin/env python3
"""Совместимость ONNX-моделей с OpenCV < 4.7: Clip(x, min, max) и Squeeze(x, axes) с входами -> с атрибутами,
Gemm с transB=0 -> веса транспонированы.

OpenCV 4.5/4.6 (Ubuntu 22.04/24.04) не читает Clip с тремя входами (так экспортированы ReLU6
в handpose_estimation_mediapipe). Использование:
    pip install onnx numpy
    python3 tools/onnx_clip_compat.py models/handpose_estimation_mediapipe_2023feb.onnx
-> рядом появится *_compat.onnx, HandTracker подхватит его сам, если оригинал не загрузился.
"""
import sys
from pathlib import Path

import numpy as np
import onnx
from onnx import helper, numpy_helper


def convert(src: Path) -> Path:
    model = onnx.load(str(src))
    consts = {i.name: numpy_helper.to_array(i) for i in model.graph.initializer}
    for node in model.graph.node:
        if node.op_type == "Constant":
            for a in node.attribute:
                if a.name == "value":
                    consts[node.output[0]] = numpy_helper.to_array(a.t)
    changed = 0
    for node in model.graph.node:
        if node.op_type != "Clip" or len(node.input) == 1:
            continue
        lo = float(np.asarray(consts[node.input[1]]).reshape(-1)[0]) if len(node.input) > 1 and node.input[1] else -3.4e38
        hi = float(np.asarray(consts[node.input[2]]).reshape(-1)[0]) if len(node.input) > 2 and node.input[2] else 3.4e38
        del node.input[1:]
        node.attribute.extend([helper.make_attribute("min", lo), helper.make_attribute("max", hi)])
        changed += 1
    for node in model.graph.node:
        if node.op_type in ("Squeeze", "Unsqueeze") and len(node.input) == 2:
            axes = [int(v) for v in np.asarray(consts[node.input[1]]).reshape(-1)]
            del node.input[1:]
            node.attribute.extend([helper.make_attribute("axes", axes)])
            changed += 1
    # Gemm с transB=0: OpenCV < 4.7 ждёт веса [out, in] — транспонируем заранее
    inits = {i.name: i for i in model.graph.initializer}
    for node in model.graph.node:
        if node.op_type != "Gemm" or len(node.input) < 2 or node.input[1] not in inits:
            continue
        attrs = {a.name: a for a in node.attribute}
        if "transB" in attrs and attrs["transB"].i == 1:
            continue
        w = numpy_helper.to_array(inits[node.input[1]])
        inits[node.input[1]].CopyFrom(numpy_helper.from_array(np.ascontiguousarray(w.T), node.input[1]))
        if "transB" in attrs:
            attrs["transB"].i = 1
        else:
            node.attribute.extend([helper.make_attribute("transB", 1)])
        changed += 1
    if changed == 0:
        print(f"{src.name}: nothing to convert")
        return src
    dst = src.with_name(src.stem + "_compat.onnx")
    onnx.save(model, str(dst))
    print(f"{src.name}: {changed} nodes -> {dst.name}")
    return dst


if __name__ == "__main__":
    for arg in sys.argv[1:]:
        convert(Path(arg))
