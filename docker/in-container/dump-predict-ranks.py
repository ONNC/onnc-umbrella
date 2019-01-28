#!/usr/bin/env python2

import argparse
from PIL import Image
from onnx import onnx
import os
import sys
import struct

def read_array_from_tensor(filename):
    tensor = onnx.TensorProto()
    with open(filename, 'rb') as file:
        tensor.ParseFromString(file.read())

    assert tensor.data_type == 1 # only allows float tensor
    assert len(tensor.dims) == 2
    assert tensor.dims[0] == 1

    # get the # of values in tensor
    size = 1
    for dim in tensor.dims[:2]:
        size *= dim

    return struct.unpack('%sf' % size, tensor.raw_data)

def read_array_from_dimg(filename):
    with open(filename, 'r') as file:
        tokens = file.readline().split(' ')[:-1]
        return [float(token) for token in tokens]

def read_array(filename):
    extension = os.path.splitext(filename)[1]
    if extension == '.dimg':
        return read_array_from_dimg(filename)
    else:
        return read_array_from_tensor(filename)

def main():
    parser = argparse.ArgumentParser(description='Display prediction ranks of given file.')
    parser.add_argument('input', type=str, metavar='INPUT',
                        help='ONNX tensor for NVDLA rawdump file')
    parser.add_argument('-n', type=int, metavar='N', default=5,
                        help='Max line of ranks')
    args = parser.parse_args()

    values = sorted(enumerate(read_array(args.input)), key=lambda x: x[1], reverse=True)
    for idx, value in values[:min(args.n, len(values))]:
        sys.stdout.write('{}: {}\n'.format(idx, value))

    return 0

if __name__ == '__main__':
    sys.exit(main())
