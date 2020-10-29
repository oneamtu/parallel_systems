#!/usr/bin/env ruby

require 'pathname'
require 'bigdecimal'
require 'pry'
require 'pry-rescue'
require 'pry-byebug'
require 'rspec/expectations'
require 'descriptive_statistics'

include RSpec::Matchers

DECIMAL="\\d+\\.?\\d+e?\\+?\\d{2}?"

def run(input, hash_workers: 1, data_workers: 0, comp_workers: 0, comp_buffered: nil, data_buffered: nil, data_use_channels: nil)
  command = "go run src/*.go --input=#{input} --hash-workers=#{hash_workers}\
    --comp-workers=#{comp_workers} --data-workers=#{data_workers} "
  command += " --data-buffered=#{data_buffered}" if data_buffered != nil
  command += " --data-use-channels=#{data_use_channels}" if data_use_channels != nil
  command += " --comp-buffered=#{comp_buffered}" if comp_buffered != nil
  command += " 2>debug.log"

  puts command
  %x(#{command})
end

def gen_test(input, answer)
  command = "go run src/*.go --input=#{input} --comp-workers=1 --data-workers=1 > #{answer}"

  puts "Generating test: #{command}"

  %x(#{command})
end

def test_format
  input = "input/simple.txt"
  answer = "answers/#{Pathname.new(input).basename.to_s.split(".").first}.ref"
  gen_test(input, answer) unless File.exist?(answer)

  seq_hash = run(input)
  expect(seq_hash.split("\n").size).to eq(1)
  expect(seq_hash).to match(/\AhashTime: \d+\.\d+µs/)

  par_hash = run(input, hash_workers: 2)
  expect(par_hash.split("\n").size).to eq(1)
  expect(par_hash).to match(/\AhashTime: \d+\.\d+µs/)

  seq_hash_group = run(input, data_workers: 1)
  expect(seq_hash_group.split("\n").size).to eq(5)
  expect(seq_hash_group.split("\n")[0]).to match(/\AhashGroupTime: \d+\.\d+µs/)

  par_hash_group = run(input, hash_workers: 2, data_workers: 2)
  expect(par_hash_group.split("\n").size).to eq(5) #TODO
  expect(par_hash_group.split("\n")[0]).to match(/\AhashGroupTime: \d+\.\d+µs/)

  seq_comp_group = run(input, data_workers: 1, comp_workers: 1)
  expect(seq_comp_group.split("\n").size).to eq(9)
  expect(seq_comp_group.split("\n")[0]).to match(/\AhashGroupTime: \d+\.\d+µs/)

  par_comp_group = run(input, hash_workers: 2, data_workers: 2, comp_workers: 2)
  expect(par_comp_group.split("\n").size).to eq(9) #TODO
  expect(par_comp_group.split("\n")[0]).to match(/\AhashGroupTime: \d+\.\d+µs/)
  expect(par_comp_group.split("\n")[5]).to match(/\AcompareTreeTime: \d+\.\d+µs/)
end

def compare_correctness(output, target)
  hash_output = output.split("\n")[1..].take_while { _1.match?(/\A\d{1,3}:/) }
  hash_target = target.split("\n")[1..].take_while { _1.match?(/\A\d{1,3}:/) }

  expect(hash_output.size).to eq(hash_target.size)

  hash_output.sort.zip(hash_target.sort).each do |o_row, t_row|
    split_o_row = o_row.split(" ")
    split_t_row = t_row.split(" ")

    expect(split_o_row[0]).to eq(split_t_row[0])
    expect(split_o_row[1..].sort).to eq(split_t_row[1..].sort)
  end

  group_output = output.split("\n")[(hash_output.size+2)..].take_while { _1.match?(/\Agroup/) }
  group_target = target.split("\n")[(hash_target.size+2)..].take_while { _1.match?(/\Agroup/) }

  group_output.map! { _1.split(" ")[2..].sort! }
  group_target.map! { _1.split(" ")[2..].sort! }

  expect(group_output.size).to eq(group_target.size)

  group_output.sort.zip(group_target.sort).each do |o_row, t_row|
    expect(o_row).to eq(t_row)
  end

rescue Exception => e
  binding.pry
end

def hash_timings
  File.open("timings/hash_times.csv", "w") do |f|
    ["input/coarse.txt", "input/fine.txt"].each do |input|
      size = %x(wc -l #{input}).to_i

      (0..Math.log(size, 2).ceil.to_i).each do |i|
        hash_workers = [1 << i, size].min

        time_mean, time_stddev = 20.times.map do
          output = run(input, hash_workers: hash_workers).scan(/\d+\.?\d+/).first
          puts output
          BigDecimal(output)
        end.then { [_1.mean, _1.standard_deviation] }

        f.puts "#{input}, #{hash_workers}, #{time_mean}, #{time_stddev}"
      end
    end
  end
end

HASH_WORKERS=32

def hash_group_timings
  File.open("timings/hash_group_times.csv", "w") do |f|
    ["input/coarse.txt", "input/fine.txt"].each do |input|
      [[1, 1, true, false],
       [HASH_WORKERS, 1, false, true],
       [HASH_WORKERS, 1, true, true],
       [HASH_WORKERS, 4, true, true],
       [HASH_WORKERS, 16, true, true],
       [HASH_WORKERS, 4, true, false],
       [HASH_WORKERS, 16, true, false],
       [HASH_WORKERS, HASH_WORKERS, true, false]].each do |hash_workers, data_workers, buffered, data_use_channels|
        time_mean, time_stddev = 20.times.map do
          output = run(
            input,
            hash_workers: hash_workers,
            data_workers: data_workers,
            data_buffered: buffered,
            data_use_channels: data_use_channels
          )
            .scan(/hashGroupTime: (#{DECIMAL})/).first
          puts output
          BigDecimal(output.first)
        end.then { [_1.mean, _1.standard_deviation] }

        if buffered
          f.puts "#{input}, #{hash_workers}, #{data_workers}, #{data_use_channels}, #{time_mean}, #{time_stddev}"
        else
          f.puts "#{input}-unbuffered, #{hash_workers}, #{data_workers}, #{data_use_channels}, #{time_mean}, #{time_stddev}"
        end
      end
    end
  end
end

def compare_tree_timings
  File.open("timings/compare_tree_times.csv", "w") do |f|
    ["input/coarse.txt", "input/fine.txt"].each do |input|
      [[1, true], [2, true], [2, false], [4, true], [8, true], [16, true]].each do |comp_workers, buffered|
        time_mean, time_stddev = 20.times.map do
          output = run(input, hash_workers: HASH_WORKERS, data_workers: 1, comp_workers: comp_workers, comp_buffered: buffered)
            .scan(/compareTreeTime: (#{DECIMAL})/).first
          puts output
          BigDecimal(output.first)
        end.then { [_1.mean, _1.standard_deviation] }

        if buffered
          f.puts "#{input}, #{comp_workers}, #{time_mean}, #{time_stddev}"
        else
          f.puts "#{input}-unbuffered, #{comp_workers}, #{time_mean}, #{time_stddev}"
        end
      end
    end
  end
end

# possible j values include 2, 4, 8, 16.
# Possible comp-workers values include 1,2,4,8,16.

def test_correctness
  Dir["input/*.txt"].each do |input|
    answer = "answers/#{Pathname.new(input).basename.to_s.split(".").first}.ref"
    gen_test(input, answer) unless File.exist?(answer)

    # compare_correctness(run(input, hash_workers: 1, data_workers: 1, comp_workers: 1), File.read(answer))
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: 1,
          comp_workers: 1),
          File.read(answer)
    )
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: HASH_WORKERS,
          comp_workers: 1),
          File.read(answer)
    )
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: HASH_WORKERS/2,
          comp_workers: 1),
          File.read(answer)
    )
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: HASH_WORKERS/2,
          comp_workers: 1,
          data_use_channels: true),
          File.read(answer)
    )
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: 1,
          comp_workers: 4),
          File.read(answer)
    )
    compare_correctness(
      run(input,
          hash_workers: HASH_WORKERS,
          data_workers: 1,
          comp_workers: 4,
          comp_buffered: true),
          File.read(answer)
    )
  end
end

# test_format
# test_correctness
# hash_timings
hash_group_timings
# compare_tree_timings
