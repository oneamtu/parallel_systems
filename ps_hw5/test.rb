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

def run(input, output, theta: 0.35, steps: 1000, delta: 0.005, n: 4, alg: "")
  if alg == ""
    command = "mpiexec -n #{n} bin/nbody -i #{input} -o #{output} -d #{delta} -t #{theta} -s #{steps} #{alg}"
  else
    command = "bin/nbody -i #{input} -o #{output} -d #{delta} -t #{theta} -s #{steps} #{alg}"
  end

  puts command
  %x(#{command})
end

def compare_correctness(output, target)
  output_lines = output.split("\n")
  target_lines = target.split("\n")

  output_lines.zip(target_lines).each do |ol, tl|
    ol.split(" ").zip(tl.split(" ")).each do |o, t|
      expect(BigDecimal(o)).to be_within(0.000001).of(BigDecimal(t))
    end
  end
rescue Exception => e
  binding.pry
end

def test_correctness(alg: "")
  [
    ["input/nb-10.txt", [1000, 5000]],
    ["input/nb-100.txt", [1000, 5000]],
  ].each do |input, n_steps|
    n_steps.each do |steps|
      [0.0, 0.5].each do |theta|
        [0.005, 0.05].each do |delta|
          answer = "answers/#{Pathname.new(input).basename.to_s.split(".").first}-#{steps}-#{theta}-#{delta}.ref"
          run(input, answer, steps: steps, delta: delta, theta: theta, alg: "-1") unless File.exist?(answer)

          tempfile = "/tmp/temppshw5.txt"
          run(input, tempfile, steps: steps, delta: delta, theta: theta, alg: alg)

          compare_correctness(
            File.read(tempfile),
            File.read(answer)
          )
        end
      end
    end
  end

  ["input/nb-100000.txt", 100, 0.5, 0.05].tap do |input, steps, theta, delta|
    answer = "answers/#{Pathname.new(input).basename.to_s.split(".").first}-#{steps}-#{theta}-#{delta}.ref"
    run(input, answer, steps: steps, delta: 0.5, theta: theta, alg: "-1") unless File.exist?(answer)

    tempfile = "/tmp/temppshw5.txt"
    run(input, tempfile, steps: steps, delta: 0.5, theta: theta, alg: "")

    compare_correctness(
      File.read(tempfile),
      File.read(answer)
    )
  end
end

def timings(speedup = true)
  [["input/nb-10.txt", 1000000],
   ["input/nb-100.txt", 100000],
   ["input/nb-100000.txt", 100]].each do |input, steps|
     sequential_out = run(input, "/dev/null", theta: 0.35, steps: steps, alg: "-1")

     seq_time = 0
     sequential_out.split("\n").each do |line|
       puts "#{input}:seq #{line}"
       seq_time = BigDecimal(line.scan(/overall: (#{DECIMAL})/).first.first) if line.match?(/overall:/)
     end


     [1, 2, 3, 4].each do |n|
       out = run(input, "/dev/null", theta: 0.35, steps: steps, n: n)

       time = 0
       out.split("\n").each do |line|
         puts "#{input}:#{n} #{line}"
         time = BigDecimal(line.scan(/overall: (#{DECIMAL})/).first.first) if line.match?(/overall:/)
       end

       puts "#{input}:#{n} speedup: #{(seq_time/time).round(2).to_s("F")}"
     end
   end
end

# test_correctness(alg: "-1")
test_correctness
# timings
