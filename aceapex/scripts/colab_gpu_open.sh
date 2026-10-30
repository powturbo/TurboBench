#!/usr/bin/env bash
# colab_gpu_open.sh - kept for the old Colab cells: the GPU run is scripts/gpu_run.sh (Colab, RunPod, any CUDA host).
set -euo pipefail
exec bash "$(dirname "$0")/gpu_run.sh" "$@"
