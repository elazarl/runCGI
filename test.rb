#!/usr/bin/env ruby
require 'minitest/autorun'
require 'yaml'
require 'set'


class TestYamlFiles < MiniTest::Unit::TestCase
	def run_test(filename)
		filename = "test/"+filename+".yaml"
		yaml = File.open filename do |f| YAML::load(f) end
		if yaml["runcgi_params"] then
			params = yaml["runcgi_params"].join " "
		else
			params = ""
		end
		expect_ar = yaml["expect"]
		expect = Set.new expect_ar
		runcgi_cmd = "./runCGI "+params+" "+filename
		result_raw = IO.popen(runcgi_cmd) do |io| io.read end
		result_formatted = (result_raw.split("\n").map {|x| "\n  "+x}).join
		result = Set.new result_raw.split("\n")
		not_in_result = ((expect-result).map{|x| "\n  "+x}).join
		assert(expect.subset?(result),"expected in #{filename}#{not_in_result}\nto be in output of #{runcgi_cmd}#{result_formatted}")
	end
	test_files = Dir.glob("test/test*.yaml").map do |f|
		File.basename(f,File.extname(f))
	end
	test_files.each do |f|
		define_method(f.to_sym) do
			run_test f
		end
	end
end
