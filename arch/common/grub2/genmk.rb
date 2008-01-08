#! /usr/bin/ruby -w
#
# Copyright (C) 2002,2003,2004,2005,2006  Free Software Foundation, Inc.
#
# This genmk.rb is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

module Enumerable
  def collect_with_index
    ret = []
    self.each_with_index do |item, index|
      ret.push(yield(item, index))
    end
    ret
  end
end

class String
  def to_var
    self.gsub(/[^a-zA-Z0-9_@]/, '_')
  end

  def suffix(str)
    self.sub(/\.[^\.]*$/, '') + '.' + str
  end

  def to_obj
    self.sub(/\.[^\.]*$/, '').to_var + '.o'
  end
end

class Image
  def initialize(dir, name)
    @dir = dir
    @name = name
  end
  attr_reader :dir, :name

  def rule(sources)
    prefix = @name.to_var
    exe = @name.suffix('exec')
    objs = sources.collect do |src|
      raise "unknown source file `#{src}'" if /\.[cS]$/ !~ src
      prefix + '-' + src.to_obj
    end
    objs_str = objs.join(' ')
    deps = objs.collect {|obj| obj.suffix('d')}
    deps_str = deps.join(' ')
    
    "CLEANFILES += #{@name} #{exe} #{objs_str}
MOSTLYCLEANFILES += #{deps_str}

#{@name}: #{exe}
	$(OBJCOPY) -O binary -R .note -R .comment $< $@

#{exe}: #{objs_str}
	$(TARGET_CC) -o $@ $^ $(TARGET_LDFLAGS) $(#{prefix}_LDFLAGS)

" + objs.collect_with_index do |obj, i|
      src = sources[i]
      fake_obj = File.basename(src).suffix('o')
      dep = deps[i]
      flag = if /\.c$/ =~ src then 'CFLAGS' else 'ASFLAGS' end
      extra_flags = if /\.S$/ =~ src then '-DASM_FILE=1' else '' end
      dir = File.dirname(src)
      
      "#{obj}: #{src}
	$(TARGET_CC) -I#{dir} -I$(srcdir)/#{dir} $(TARGET_CPPFLAGS) #{extra_flags} $(TARGET_#{flag}) $(#{prefix}_#{flag}) -MD -c -o $@ $<
-include #{dep}

"
    end.join('')
  end
end

# Use PModule instead Module, to avoid name conflicting.
class PModule
  def initialize(dir, name)
    @dir = dir
    @name = name
  end
  attr_reader :dir, :name

  def rule(sources)
    prefix = @name.to_var
    objs = sources.collect do |src|
      raise "unknown source file `#{src}'" if /\.[cS]$/ !~ src
      prefix + '-' + src.to_obj
    end
    objs_str = objs.join(' ')
    deps = objs.collect {|obj| obj.suffix('d')}
    deps_str = deps.join(' ')
    pre_obj = 'pre-' + @name.suffix('o')
    mod_src = 'mod-' + @name.suffix('c')
    mod_obj = mod_src.suffix('o')
    defsym = 'def-' + @name.suffix('lst')
    undsym = 'und-' + @name.suffix('lst')
    mod_name = File.basename(@name, '.mod')
    symbolic_name = mod_name.sub(/\.[^\.]*$/, '')
    
    "CLEANFILES += #{@name} #{mod_obj} #{mod_src} #{pre_obj} #{objs_str} #{undsym}
ifneq ($(#{prefix}_EXPORTS),no)
CLEANFILES += #{defsym}
DEFSYMFILES += #{defsym}
endif
MOSTLYCLEANFILES += #{deps_str}
UNDSYMFILES += #{undsym}

#{@name}: #{pre_obj} #{mod_obj}
	-rm -f $@
	$(TARGET_CC) $(#{prefix}_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ $^
	$(STRIP) --strip-unneeded -K grub_mod_init -K grub_mod_fini -R .note -R .comment $@

#{pre_obj}: $(#{prefix}_DEPENDENCIES) #{objs_str}
	-rm -f $@
	$(TARGET_CC) $(#{prefix}_LDFLAGS) $(TARGET_LDFLAGS) -Wl,-r,-d -o $@ #{objs_str}

#{mod_obj}: #{mod_src}
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(#{prefix}_CFLAGS) -c -o $@ $<

#{mod_src}: moddep.lst genmodsrc.sh
	sh $(srcdir)/genmodsrc.sh '#{mod_name}' $< > $@ || (rm -f $@; exit 1)

ifneq ($(#{prefix}_EXPORTS),no)
#{defsym}: #{pre_obj}
	$(NM) -g --defined-only -P -p $< | sed 's/^\\([^ ]*\\).*/\\1 #{mod_name}/' > $@
endif

#{undsym}: #{pre_obj}
	echo '#{mod_name}' > $@
	$(NM) -u -P -p $< | cut -f1 -d' ' >> $@

" + objs.collect_with_index do |obj, i|
      src = sources[i]
      fake_obj = File.basename(src).suffix('o')
      command = 'cmd-' + obj.suffix('lst')
      fs = 'fs-' + obj.suffix('lst')
      dep = deps[i]
      flag = if /\.c$/ =~ src then 'CFLAGS' else 'ASFLAGS' end
      extra_flags = if /\.S$/ =~ src then '-DASM_FILE=1' else '' end
      dir = File.dirname(src)

      "#{obj}: #{src}
	$(TARGET_CC) -I#{dir} -I$(srcdir)/#{dir} $(TARGET_CPPFLAGS) #{extra_flags} $(TARGET_#{flag}) $(#{prefix}_#{flag}) -MD -c -o $@ $<
-include #{dep}

CLEANFILES += #{command} #{fs}
COMMANDFILES += #{command}
FSFILES += #{fs}

#{command}: #{src} gencmdlist.sh
	set -e; \
	  $(TARGET_CC) -I#{dir} -I$(srcdir)/#{dir} $(TARGET_CPPFLAGS) $(TARGET_#{flag}) $(#{prefix}_#{flag}) -E $< \
	  | sh $(srcdir)/gencmdlist.sh #{symbolic_name} > $@ || (rm -f $@; exit 1)

#{fs}: #{src} genfslist.sh
	set -e; \
	  $(TARGET_CC) -I#{dir} -I$(srcdir)/#{dir} $(TARGET_CPPFLAGS) $(TARGET_#{flag}) $(#{prefix}_#{flag}) -E $< \
	  | sh $(srcdir)/genfslist.sh #{symbolic_name} > $@ || (rm -f $@; exit 1)


"
    end.join('')
  end
end

class Utility
  def initialize(dir, name)
    @dir = dir
    @name = name
  end
  attr_reader :dir, :name

  def rule(sources)
    prefix = @name.to_var
    objs = sources.collect do |src|
      raise "unknown source file `#{src}'" if /\.[cS]$/ !~ src
      prefix + '-' + src.to_obj
    end
    objs_str = objs.join(' ');
    deps = objs.collect {|obj| obj.suffix('d')}
    deps_str = deps.join(' ');

    "CLEANFILES += #{@name} #{objs_str}
MOSTLYCLEANFILES += #{deps_str}

#{@name}: $(#{prefix}_DEPENDENCIES) #{objs_str}
	$(CC) -o $@ #{objs_str} $(LDFLAGS) $(#{prefix}_LDFLAGS)

" + objs.collect_with_index do |obj, i|
      src = sources[i]
      fake_obj = File.basename(src).suffix('o')
      dep = deps[i]
      dir = File.dirname(src)

      "#{obj}: #{src} $(#{src}_DEPENDENCIES)
	$(CC) -I#{dir} -I$(srcdir)/#{dir} $(CPPFLAGS) $(CFLAGS) -DGRUB_UTIL=1 $(#{prefix}_CFLAGS) -MD -c -o $@ $<
-include #{dep}

"
    end.join('')
  end
end

class Program
  def initialize(dir, name)
    @dir = dir
    @name = name
  end
  attr_reader :dir, :name

  def rule(sources)
    prefix = @name.to_var
    objs = sources.collect do |src|
      raise "unknown source file `#{src}'" if /\.[cS]$/ !~ src
      prefix + '-' + src.to_obj
    end
    objs_str = objs.join(' ');
    deps = objs.collect {|obj| obj.suffix('d')}
    deps_str = deps.join(' ');

    "CLEANFILES += #{@name} #{objs_str}
MOSTLYCLEANFILES += #{deps_str}

#{@name}: $(#{prefix}_DEPENDENCIES) #{objs_str}
	$(TARGET_CC) -o $@ #{objs_str} $(TARGET_LDFLAGS) $(#{prefix}_LDFLAGS)

" + objs.collect_with_index do |obj, i|
      src = sources[i]
      fake_obj = File.basename(src).suffix('o')
      dep = deps[i]
      dir = File.dirname(src)

      "#{obj}: #{src}
	$(TARGET_CC) -I#{dir} -I$(srcdir)/#{dir} $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) $(#{prefix}_CFLAGS) -MD -c -o $@ $<
-include #{dep}

"
    end.join('')
  end
end

class Script
  def initialize(dir, name)
    @dir = dir
    @name = name
  end
  attr_reader :dir, :name

  def rule(sources)
    if sources.length != 1
      raise "only a single source file must be specified for a script"
    end
    src = sources[0]
    if /\.in$/ !~ src
      raise "unknown source file `#{src}'" 
    end

    "CLEANFILES += #{@name}

#{@name}: #{src} config.status
	./config.status --file=#{name}:#{src}
	chmod +x $@

"
  end
end

images = []
utils = []
pmodules = []
programs = []
scripts = []

cont = false
s = nil
while l = gets
  if cont
    s += l
  else
    s = l
  end

  print l
  cont = (/\\$/ =~ l)
  unless cont
    s.gsub!(/\\\n/, ' ')
    
    if /^([a-zA-Z0-9_]+)\s*\+?=\s*(.*?)\s*$/ =~ s
      var, args = $1, $2

      if var =~ /^([a-zA-Z0-9_]+)_([A-Z]+)$/
	prefix, type = $1, $2

	case type
	when 'IMAGES'
	  images += args.split(/\s+/).collect do |img|
	    Image.new(prefix, img)
	  end

	when 'MODULES'
	  pmodules += args.split(/\s+/).collect do |pmod|
	    PModule.new(prefix, pmod)
	  end
	  
	when 'UTILITIES'
	  utils += args.split(/\s+/).collect do |util|
	    Utility.new(prefix, util)
	  end

	when 'PROGRAMS'
	  programs += args.split(/\s+/).collect do |prog|
	    Program.new(prefix, prog)
	  end

	when 'SCRIPTS'
	  scripts += args.split(/\s+/).collect do |script|
	    Script.new(prefix, script)
	  end

	when 'SOURCES'
	  if img = images.detect() {|i| i.name.to_var == prefix}
	    print img.rule(args.split(/\s+/))
	  elsif pmod = pmodules.detect() {|m| m.name.to_var == prefix}
	    print pmod.rule(args.split(/\s+/))
	  elsif util = utils.detect() {|u| u.name.to_var == prefix}
	    print util.rule(args.split(/\s+/))
	  elsif program = programs.detect() {|u| u.name.to_var == prefix}
	    print program.rule(args.split(/\s+/))
	  elsif script = scripts.detect() {|s| s.name.to_var == prefix}
	    print script.rule(args.split(/\s+/))
	  end
	end
      end
      
    end
    
  end
  
end

