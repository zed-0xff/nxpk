#!/usr/bin/env ruby
# coding: binary
# frozen_string_literal: true

require 'iostruct'
require 'zhexdump'
require 'awesome_print'
require 'fileutils'
require 'zlib'

@total_nfiles = 0
@total_nunknown = 0

module NXPK
  class Header < IOStruct.new('a4L5', :magic, :n, :a, :b, :c, :dir_offset)
    def valid?
      magic == 'NXPK'
    end
  end

  class DirEntry < IOStruct.new('L7', :key, :offset, :comp_size, :raw_size, :hash1, :hash2, :comp_method)
    COMP_NONE = 0
    COMP_ZLIB = 1

    def inspect
      if comp_size == raw_size && hash1 == hash2
        "<DirEntry key=%08x offset=%08x size=%08x hash=%08x comp_method=%x>" % [ key, offset, comp_size, hash1, comp_method ]
      else
        "<DirEntry key=%08x offset=%08x comp_size=%08x raw_size=%08x hash1=%08x hash2=%08x comp_method=%x>" % [ key, offset, comp_size, raw_size, hash1, hash2, comp_method ]
      end
    end

    def decompress data
      case comp_method
      when COMP_NONE
        data
      when COMP_ZLIB
        Zlib::Inflate.inflate(data)
      else
        raise "Unknown compression method: #{comp_method}"
      end
    end
  end

  class Archive
    attr_reader :header, :entries

    def initialize(io)
      @header = Header.read(io)
      raise 'Invalid NXPK header' unless @header.valid?

      io.seek @header.dir_offset
      @entries = @header.n.times.map{ DirEntry.read(io) }
    end
  end

  DW = 0xffff_ffff

  def self.mask32(x)
    x & DW
  end

  def self.calc_hash(s)
    s += "\x00" * (4 - s.size % 4) if s.size % 4 != 0
    a = s.unpack('L*') + [0x9BE74448, 0x66F42C48]

    v = 0xF4FA8928
    edi = 0x7758B42B
    esi = 0x37A8470E
    edx = 0
    num = 0

    a.each do |eax|
      v = (v << 1) | (v >> 0x1F)
      ebx = 0x267B0B11 ^ v
      esi ^= eax
      edi ^= eax
      num = esi * (((ebx + edi) | 0x02040801) & 0xBFEF7FDF)

      edx, eax = num.divmod(DW+1)
      num = (edx > 0 ? 1 : 0) + eax + edx

      eax = mask32(num) + (num > DW ? 1 : 0)
      num = edi * (((ebx + esi) | 0x00804021) & 0x7DFEFBFF)
      esi = eax

      edx, eax = num.divmod(DW+1)
      num = edx * 2
      num = mask32(num) + (num > DW ? 1 : 0) + eax
      edi = mask32(num) + (num > DW ? 2 : 0)
    end
    esi ^ edi
  end
end

# map first 4 bytes to extension
FAST_MAGIC = {
  "\x89PNG" => 'png',
  "\xFF\xD8\xFF\xE0" => 'jpg', # maybe just \xFF\xD8
  "\xFF\xD8\xFF\xE1" => 'jpg', # maybe just \xFF\xD8
  "\x03\xF3\r\n" => 'pyc',
  "\x34\x80\xc8\xbb" => 'mesh',
  "\xabKTX" => 'ktx',
  "%YAM" => 'controller',
  "BM8\x00" => 'bmp',
  "GIF8" => 'gif',
  'DDS ' => 'dds',
  'NTRK' => 'trk',
  'RGIS' => 'gis',
  'TRUE' => 'tga',
}.merge(
  %w'CVIS MDMP'.map{ |x| [x, x.downcase] }.to_h
)

class String
  def printable_ascii_only?
    self =~ /\A[\r\n\x09\x20-\x7E]*\z/
  end
end

def is_atlas?(data)
  a = data.split("\n", 4)
  a.size == 4 &&
    a[0].strip =~ /^[\d ]+$/ &&
    a[1].strip =~ /^\d+$/ &&
    a[2].strip =~ /^\d+$/ &&
    a[3] =~ /\.(png|tga|dds)/
end

# maybe too wide
def is_shader?(data)
  data['#include <'] ||
    data['float '] ||
    data['float4'] ||
    data[/^void /] ||
    data['#define']
end

def is_python?(data)
    data.start_with?("import ") ||
      data.start_with?(/\s*__all__\s*=/m) ||
      data.start_with?("#!/usr/bin/env python") ||
      data[/coding\s*:/] ||
      data[/^def \w/]
end

def guess_ext(data)
  ext = FAST_MAGIC[data[0, 4]]
  return ext if ext

  case
  when data.start_with?("\xef\xbb\xbf")
    return guess_ext(data[3..-1])
  when is_python?(data)
    'py'
  when data.start_with?(/data\s*=/)
    'js'
  when data[8, 4] == 'VAND'
    'vand'
  when data =~ /\A\s*</m && data =~ />\s*\z/m
    # maybe too wide
    'xml'
  when is_shader?(data)
    'shader'
  else
    data.force_encoding('utf-8')
    if data.valid_encoding?
      return "shader" if data["\n{"] && data["\n}"]
      return "atlas" if is_atlas?(data)

      tmp = data.strip
      return 'json' if tmp[0] == '{' && tmp[-1] == '}'
    end

    data.force_encoding('binary')
    if data.printable_ascii_only?
      #puts data[0,0x1000].yellow
      'txt'
    else
      #Zhexdump.dump(data[0, 0x40])
      'bin'
    end
  end
end

def process_file(fname, mode)
  if mode == :extract
    printf "[*] %-30s .. ", fname
    $stdout.flush
  end
  keys = load_keys(fname)

  nunknown = nall = 0
  basedir = File.join("out", File.basename(fname))
  File.open(fname, 'rb') do |f|
    archive = NXPK::Archive.new(f)

    archive.entries.each do |e|
      fname = keys[e.key] || @all_keys[e.key]

      if mode == :list_keys
        printf "%08x %s\n", e.key, fname
        next
      end

      f.seek e.offset
      data = f.read(e.comp_size)
      data = e.decompress(data)

      unless fname
        ext = guess_ext(data)
        fname = File.join("_#{ext}", "%08x.%s" % [e.key, ext])
        nunknown += 1
      end

      if mode == :list
        printf "[.] %8d %08x %s\n", data.size, e.key, fname
        next
      end

      fname.tr!("\\", '/')
      fname.sub!(/^[a-z]:/i, '')
      fname.sub!(/^\/+/, '')

      raise "[!] invalid filename: #{fname}" if fname[0] == '/' || fname['..'] || fname["\\"]

      outname = File.join(basedir, fname)
      FileUtils.mkdir_p(File.dirname(outname))

      #puts "[.] #{outname}"
      File.binwrite(outname, data)
      nall += 1
    end
  end

  if mode == :extract
    printf "extracted %5d files, %5d unknown names\n", nall, nunknown
    @total_nfiles += nall
    @total_nunknown += nunknown
  end
end

def load_keys fname
  keys_dir = File.expand_path('keys', __dir__)
  fname = File.join(keys_dir, File.basename(fname)).sub(/\.\w+$/, '') + '.keys'
  fname = File.join(keys_dir, "all.keys") if !File.exist?(fname) || File.size(fname) == 0

  #$stderr.puts "[*] Using keys from #{fname}"
  return {} unless File.exist?(fname)

  r = {}
  File.read(fname).each_line do |line|
    line.strip!
    next if line[0] == '#'

    key, value = line.split(' ', 2)
    r[key.to_i(16)] = value
  end
  r
end

if $0 == __FILE__
  case ARGV.shift
  when 'k'
    mode = :list_keys
  when 'l'
    mode = :list
  when 'x'
    mode = :extract
  else
    puts "Usage: #{File.basename($0)} <mode> file.nxpk ..."
    puts " mode: k - list in .keys format"
    puts "       l - list"
    puts "       x - extract"
    exit 1
  end

  @all_keys = load_keys('all')
  ARGV.each do |fname|
    next if File.directory?(fname)

    process_file fname, mode
  end

  if mode == :extract && ARGV.size > 1
    printf "[=] %-30s    ", "TOTAL"
    printf "          %5d files, %5d unknown names\n", @total_nfiles, @total_nunknown
  end
end
