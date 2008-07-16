function object=parseelf(path)
fid = fopen(path,'r');
%elf header
object.elf_header=readheader(fid);
%program header
if object.elf_header.e_phoff > 0
	fseek(fid,object.elf_header.e_phoff);
	object.program_headers = readprogramheaders(fid,object.elf_header.e_phentsize,object.elf_header.e_phnum);
end
%section headers
if object.elf_header.e_shoff > 0
	fseek(fid,object.elf_header.e_shoff);
	object.section_headers = readsectionheaders(fid,object.elf_header.e_shentsize,object.elf_header.e_shnum);
	object.strtabindex=find([object.section_headers.sh_type]==3);
	object.symtabindex=find([object.section_headers.sh_type]==2);
	%get strtabs
	if numel(object.strtabindex)>0
		object.strtabs={object.section_headers(object.strtabindex).strings};
		object.stroffsets={object.section_headers(object.strtabindex).stroffsets};
	end
	%fill in section names
	if object.elf_header.e_shstrndx>0
	  fprintf('getting section names...\n')
		snamesindex = find(object.strtabindex==(object.elf_header.e_shstrndx+1));
		object.section_names = object.strtabs{snamesindex};
		snameoffsets = object.stroffsets{snamesindex};
		for i=1:numel(object.section_headers)
			object.section_headers(i).name = getname(object,snamesindex,object.section_headers(i).sh_name);
		end
		object.section_headers(1).name = 'UNDEF';
	end
	%get symbols and fill in names
	if numel(object.symtabindex)>0
		object.symtabs={object.section_headers(object.symtabindex).symbols};
		%find .strtab
		strtabindex=find(cellfun(@(c)(numel(c)==1&&c==1),strfind({object.section_headers.name},'.strtab')));
		if numel(strtabindex)==1		
		  strtabindex = find(object.strtabindex==strtabindex);
		  fprintf('getting symbol names...\n')
		  for i=1:numel(object.symtabs)
		    for j=1:numel(object.symtabs{i})
		      object.symtabs{i}(j).name = getname(object,strtabindex,object.symtabs{i}(j).st_name);
		      shindex = object.symtabs{i}(j).st_shndx;
		      if (shindex > 0)
		        object.symtabs{i}(j).abs_offset = object.symtabs{i}(j).st_value + object.section_headers(shindex+1).sh_offset;
		        object.symtabs{i}(j).section_name = object.section_headers(shindex+1).name;
		      else
		        object.symtabs{i}(j).abs_offset = -1;
	        end
	      end
	    end
	  end
	end
end
fclose(fid);

function name=getname(object,tabindex,sh_name)
if sh_name <=0
	name = '';
	return;
end
snameoffsets = object.stroffsets{tabindex};
nameindex=max(find(sh_name>=snameoffsets));
nameoffset=sh_name-snameoffsets(nameindex)+1;
str = object.strtabs{tabindex}{nameindex};
name = str(nameoffset:end);

function sheaders = readsectionheaders(fid,s,n)
fprintf('reading sections... ');fflush(stdout);
for i=1:n
fprintf(' %i ',i);fflush(stdout);
pos = ftell(fid);
sheaders(i).sh_name=fread(fid,1,'int32');
sheaders(i).sh_type=fread(fid,1,'int32');
sheaders(i).sh_flags=fread(fid,1,'int32');
sheaders(i).sh_addr=fread(fid,1,'int32');
sheaders(i).sh_offset=fread(fid,1,'int32');
sheaders(i).sh_size=fread(fid,1,'int32');
sheaders(i).sh_link=fread(fid,1,'int32');
sheaders(i).sh_info=fread(fid,1,'int32');
sheaders(i).sh_addralign=fread(fid,1,'int32');
sheaders(i).sh_entsize=fread(fid,1,'int32');
switch(sheaders(i).sh_type)
	case 0
		sheaders(i).typestr = 'null';
	case 1
		sheaders(i).typestr = 'progbits';
	case 2
		sheaders(i).typestr = 'symtab';
	case 3
		sheaders(i).typestr = 'strtab';
	case 4
		sheaders(i).typestr = 'rela';
	case 5
		sheaders(i).typestr = 'hash';
	case 6
		sheaders(i).typestr = 'danymic';
	case 7
		sheaders(i).typestr = 'note';
	case 8
		sheaders(i).typestr = 'nobits';
	case 9
		sheaders(i).typestr = 'rel';
	case 10
		sheaders(i).typestr = 'shlib';
	case 11
		sheaders(i).typestr = 'dynsym';
	otherwise
		sheaders(i).typestr = 'unknown';
end
if sheaders(i).sh_type == 3
  fprintf('str ');fflush(stdout);
	fseek(fid,sheaders(i).sh_offset+1);
	[sheaders(i).strings,sheaders(i).stroffsets] = readstrings(fid,sheaders(i).sh_offset+sheaders(i).sh_size);
fseek(fid,pos+s);
elseif sheaders(i).sh_type == 2
  fprintf('sym ');fflush(stdout);
	fseek(fid,sheaders(i).sh_offset);
	sheaders(i).symbols = readsymbols(fid,sheaders(i).sh_offset+sheaders(i).sh_size);
end
fseek(fid,pos+s);
end
fprintf('done\n');fflush(stdout);

function symbols = readsymbols(fid,end_offset)
	symbols = [];
	do
		symbol.st_name = fread(fid,1,'int32');
		symbol.st_value = fread(fid,1,'int32');
		symbol.st_size = fread(fid,1,'int32');
		symbol.st_info = fread(fid,1,'uchar');
		symbol.st_other = fread(fid,1,'uchar');
		symbol.st_shndx = fread(fid,1,'int16');
		if isempty(symbols)
			symbols = symbol;
		else
			symbols(end+1) = symbol;			
		end
	until ftell(fid)>=end_offset

function [strings,offsets] = readstrings(fid,end_offset)
	strings = {};
	offsets = [];
	offset=1;
	do
		str = '';
		offsets(end+1) = offset;
		%TODO: use fscanf('%s',1);
		while	(c = char(fread(fid,1,'uchar'))) != 0
			str(end+1) = c;
			offset = offset + 1;
		end
		offset = offset + 1;
		strings{end+1} = str;
	until ftell(fid)>=end_offset

function pheaders = readprogramheaders(fid,s,n)
fprintf('reading program headers... ');fflush(stdout);
for i=1:n
fprintf('%i, ',i);fflush(stdout);
pos = ftell(fid);
pheaders(i).p_type=fread(fid,1,'int32');
pheaders(i).p_offsset=fread(fid,1,'int32');
pheaders(i).p_vaddr=fread(fid,1,'int32');
pheaders(i).p_paddr=fread(fid,1,'int32');
pheaders(i).p_filesz=fread(fid,1,'int32');
pheaders(i).p_memsz=fread(fid,1,'int32');
pheaders(i).p_flags=fread(fid,1,'int32');
pheaders(i).p_align=fread(fid,1,'int32');
fseek(fid,pos+s)
end
fprintf('done\n');fflush(stdout);

function header=readheader(fid)
fprintf('reading elf-header...\n');fflush(stdout);
header.e_ident=fread(fid,16,'uchar');
header.e_type=fread(fid,1,'int16');
header.e_machine=fread(fid,1,'int16');
header.e_version=fread(fid,1,'int32');
header.e_entry=fread(fid,1,'int32');
header.e_phoff=fread(fid,1,'int32');
header.e_shoff=fread(fid,1,'int32');
header.e_flags=fread(fid,1,'int32');
header.e_ehsize=fread(fid,1,'int16');
header.e_phentsize=fread(fid,1,'int16');
header.e_phnum=fread(fid,1,'int16');
header.e_shentsize=fread(fid,1,'int16');
header.e_shnum=fread(fid,1,'int16');
header.e_shstrndx=fread(fid,1,'int16');
switch header.e_type
	case 0
		header.typestr = 'none';
	case 1
		header.typestr = 'relocatable';
	case 2
		header.typestr = 'executable';
	case 3
		header.typestr = 'shared object-file';
	case 4
		header.typestr = 'core dump';
	otherwise
		header.typestr = 'unknown';
end
switch header.e_machine
	case 0
		header.machinestr = 'none';
	case 1
		header.machinestr = 'at&t we 32100';
	case 2
		header.machinestr = 'SPARC';
	case 3
		header.machinestr = 'Intel';
	case 4
		header.machinestr = 'M68000';
	case 5
		header.machinestr = 'M88000';
	case 7
		header.machinestr = 'i80860';
	case 8
		header.machinestr = 'MIPS RS3000 Big-Endian';
	case 10
		header.machinestr = 'MIPS RS4000 Little-Endian';
	case {11,12,13,14,15,16}
		header.machinestr = 'reserved';
	otherwise
		header.machinestr = 'unknown';
end
switch header.e_version
	case 0
		header.versionstr = 'invalid';
	case 1
		header.versionstr = 'current';
	otherwise
		header.versionstr = 'unknown';
end
if all(header.e_ident(1:4)==[0x7f 'ELF']')
	header.magicstate = 'ok';
else
	header.magicstate = 'invalid';
end
switch header.e_ident(5)
	case 0
		header.classstr = 'invalid';
	case 1
		header.classstr = '32bit';
	case 2
		header.classstr = '64bit';
	otherwise
		header.versionstr = 'unknown';
end
switch header.e_ident(6)
	case 0
		header.datastr = 'invalid';
	case 1
		header.datastr = 'lsb';
	case 2
		header.datastr = 'msb';
	otherwise
		header.versionstr = 'unknown';
end
