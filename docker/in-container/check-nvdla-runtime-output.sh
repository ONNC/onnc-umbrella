#!/usr/bin/env bash

function show_usage
{
  echo "Usage of $(basename $0)"
  echo
  echo "  $0 [ONNX model] [input ONNX tensor] [output ONNX tensor]"
  echo
}

if [ $# -lt 3 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
  show_usage
  exit 1
fi

onnc_home=/onnc/onnc
onnc_scripts=$onnc_home/scripts

model="$1"
input="$2"
output="$3"

work_dir=/usr/local/nvdla

# prepare nvdla loadable
onnc -mquadruple nvdla $model
mv out.nvdla $work_dir/model.nvdla

# prepare nvdla input image
$onnc_scripts/convert-tensor-to-jpeg.py $input -o $work_dir/input.jpg
nvdla-runtime.expect root nvdla /mnt/model.nvdla /mnt/input.jpg

# check nvdla output
diff <(dump-predict-ranks.py $work_dir/output.dimg) <(dump-predict-ranks.py $output)
exit $?
