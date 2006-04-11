#! /usr/bin/ruby -w
#
# Copyright (C) 2003  Free Software Foundation, Inc.
#
# This unifont2pff.rb is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# The magic number of the font file.
MAGIC = "PPF\x7f"

def usage(status = 0)
  puts "Usage: ruby unifont2pff.rb [RANGE...] FILE"
  exit(status)
end

file = ARGV.pop

ranges = []
ARGV.each do |range|
  if /\A([0-9a-fA-F]+):([0-9a-fA-F]+)\z/ =~ range
    ranges << [$1.hex, $2.hex]
  elsif /\A([0-9a-fA-F]+)\z/ =~ range
    ranges << [$1.hex, $1.hex]
  else
    usage(1)
  end
end

def ranges.contain?(code)
  if self.empty?
    true
  else
    self.each do |r|
      return true if r[0] <= code and r[1] >= code
    end
    false
  end
end

fonts = []
IO.foreach(file) do |line|
  if /^([0-9A-F]+):([0-9A-F]+)$/ =~ line
    code = $1.hex
    next unless ranges.contain?(code)
    
    bitmap = $2
    if bitmap.size != 32 and bitmap.size != 64
      raise "invalid bitmap size: #{bitmap}"
    end

    fonts << [code, bitmap]
  else
    raise "invalid line format: #{line}"
  end
end

fonts.sort! {|a,b| a[0] <=> b[0]}

# Output the result.
print MAGIC
print [fonts.size].pack('V')

offset = 8 + fonts.size * 8
fonts.each do |f|
  print [f[0]].pack('V')
  print [offset].pack('V')
  offset += 4 + 16 * f[1].size / 32
end

fonts.each do |f|
  print [f[1].size / 32].pack('V')
  print f[1].scan(/../).collect {|a| a.hex}.pack('C*')
end
