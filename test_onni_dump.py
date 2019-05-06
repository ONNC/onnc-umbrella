#!/usr/bin/env python
from __future__ import print_function
from onnx import numpy_helper
from io import StringIO
from subprocess import check_output, CalledProcessError

import shlex
import numpy
import onnx
import sys
import os
import io
import re


model_path = '/models/'

class TestOnniDump:


	def discovery_testcase(self, model_name):
		"""
		find the test cases under the model_path/model_name

		Parameters
		==========
		model_name: the name of model under which test cases will be discovered
		"""
		test_cases = []
		for fn in os.listdir(os.path.join(model_path, model_name)):
			if fn.startswith('test') and os.path.isdir(os.path.join(model_path, model_name, fn)):
				test_cases.append(fn)
		return test_cases

	def read_output_pb(self, filename):
		"""
		parse the protobuf file into onnx.tensor

		Parameters
		==========
		filename: the filename pointing to a protobuf pb file
		"""
		tensor = onnx.TensorProto()
		with io.open(filename, 'rb') as f:
			tensor.ParseFromString(f.read())
		true_array = numpy_helper.to_array(tensor)
		return true_array

	def parse_onni_stdout(self, logfile='output_0.log'):
		"""
		This function parse the stdout (if outputing array)
		
		Parameters
		==========
		logfile: the filename pointing to the stdout dump
		"""
		buf = None
		with io.open(logfile, encoding='utf-8') as fp:
			for line in fp:
				if not line.startswith('[v') and line:
					buf = StringIO(re.sub(r'[^\d.+-]', ' ', line, flags=re.U))
		if buf:
			output_array = numpy.fromstring(buf.getvalue(), sep=" ")
			return output_array

	def test_stdout(self, model_name, testcase_name):
		"""
		test the single testcase given the model name through the stdout dump

		Parameters
		==========
		model_name: the name of testing model
		testcase_name: testcase name of testing model
		"""
		truepb_fn = os.path.join(model_path, model_name, testcase_name, 'output_0.pb')
		true_array = case.read_output_pb(truepb_fn)
		outpb_fn = os.path.join('output-%s-%s.log' % (model_name, testcase_name))
		output_array = case.parse_onni_stdout(outpb_fn)
		output_array = output_array.reshape(true_array.shape).astype(true_array.dtype)
		numpy.testing.assert_allclose(true_array, output_array)	

	def test_pb(self, model_name, testcase_name):
		"""
		test the single testcase given the model name through the output protobuf written 
		on disk

		Parameters
		==========
		model_name: the name of testing model
		testcase_name: testcase name of testing model
		"""
		truepb_fn = os.path.join(model_path, model_name, testcase_name, 'output_0.pb')
		true_array = case.read_output_pb(truepb_fn)
		outpb_fn = os.path.join('output-%s-%s.pb' % (model_name, testcase_name))
		output_array = case.read_output_pb(outpb_fn)
		numpy.testing.assert_allclose(true_array, output_array)	


if __name__ == '__main__':
	model_name = sys.argv[1]
	case = TestOnniDump()
	test_cases = case.discovery_testcase(model_name)
	
	numcase_of_failing = 0	 
	for case_name in test_cases:
		cmd = "onni /models/{model}/model.onnx /models/{model}/{testcase}/input_0.pb" \
	          " -o output-{model}-{testcase}.pb".format(
	          model=model_name, testcase=case_name) 
		try:
			output = check_output(shlex.split(cmd), universal_newlines=True)	
		except CalledProcessError as e:
			if e.returncode >= 0:
				print(e.stderr)
		else:
			with open('output-{model}-{testcase}.log'.format(
					model=model_name, testcase=case_name), 'wt') as fp:
					fp.write(output)

		try:
			case.test_stdout(model_name, case_name)
		except AssertionError:
			numcase_of_failing += 1

	assert(numcase_of_failing == 0)