#! /bin/perl
#
# Extends include-header-files with __attribute__((packed))
# to make ixemul and os-includes ppc-compatible
#
# 07.06.98 Samuel Devulder: __attribute((aligned(2)) for all >2 byte elements
# 18.06.98 Holger Jakob:    It is not enough to use a modified
#                           exec/types.h only :(
# 21.05.2000 Emmanuel Lesueur: don't put __attribute__((aligned(2)) for structure
#                              elements that are on correct boudaries.
#
require 5.002;

# structs that have a length non multiple of 4:

%oddstructs=(
	AChain => 2,
	AnchorPath => 1,
	AnimComp => 2,
	AnimOb => 2,
	AppMessage => 2,
	AvailFonts => 2,
	AvailFontsHeader => 2,
	CardHandle => 1,
        CDInfo => 2,
	CIA => 2,
	ClipboardUnitPartial => 2,
	ClockData => 2,
	ColorRegister => 1,
	Conductor => 1,
	CopIns => 2,
	CopList => 2,
	Custom => 2,
	DTSpecialInfo => 2,
	DataType => 2,
	DateTime => 2,
	Device => 2,
	DeviceTData => 2,
	DiscResourceUnit => 2,
	DiskFontHeader => 2,
	DiskObject => 2,
	DosInfo => 2,
	DosLibrary => 2,
	DrawInfo => 2,
	DrawerData => 2,
	FileSysEntry => 2,
	GadgetInfo => 2,
	GlyphWidthEntry => 2,
	IEPointerTablet => 2,
	IODRPReq => 2,
	IOExtPar => 2,
	IOExtSer => 2,
	IOPrtCmdReq => 2,
	InputEvent => 2,
	Interrupt => 2,
	Isrvstr => 2,
	KeyMapNode => 2,
	Layer_Info => 2,
	Library => 2,
	List => 2,
	MPEGBorderParams => 1,
	Menu => 2,
	MsgPort => 2,
	Node => 2,
	NotifyMessage => 2,
	PrefHeader => 2,
	PrinterData => 2,
	PrinterExtendedData => 2,
	PrinterGfxPrefs => 2,
	PrinterSegment => 2,
	PrtInfo => 2,
	PubScreenNode => 1,
	RGBTable => 1,
	Resident => 2,
	RexxTask => 2,
	SCSICmd => 2,
	SatisfyMsg => 2,
	SerialPrefs => 1,
	SignalSemaphore => 2,
	SpecialMonitor => 2,
	TAvailFonts => 2,
	TOCEntry => 2,
	TOCSummary => 2,
	ToolNode => 2,
	Unit => 2,
	View => 2,
	ViewPortExtra => 2,
	bltnode => 2,
	cprlist => 2,
);

undef $/;

die "no include" if not $include=@ARGV[0];
#print STDERR "Copying $include ... ";

open(INP,"<$include");
$source=<INP>;

# Check for additional patches here...
# user.h, (screens.h), setjmp.h...

#if( $include=~ /include\/user.h$/i ) { $source=~ s/(\W)jmp_buf(\W)/$1jmp_buf_m68k$2/g; }
#if( $include=~ /include\/setjmp.h$/i ) {
#  $source=~ s/(#define\s+_JBLEN)(\s+\d+)/$1_M68K $2\n$1\t\t26+17*2/;
#  $source=~ s/(typedef\s+int\s+sigjmp_buf)(.*)(_JBLEN)(.*)/$1_m68k$2$3_M68K$4\n$1$2$3$4/;
#  $source=~ s/(typedef\s+int\s+jmp_buf)(.*)(_JBLEN)(.*)/$1_m68k$2$3_M68K$4\n$1$2$3$4/;
#}
if( $include=~ /include\/sys\/syscall.h$/i ) { $source=~ s/#ifndef\s+?_KERNEL(.*?)#endif//s; }

#
#

$source=~ s/\/\*.*?\*\///sg;  # Sorry, no comments
$source=~ s/^(\s*)struct((.|\n)*?)({(.|\n)*?});/&ins_packed_struct($2,$4)/meg;
# ToDo: same for typdef
#$source=~ s/^typedef((.|\n)*?)((\w|\n)*?);/&ins_packed_typedef($1,$3)/meg;
print $source;

close(INP);
#print STDERR "Applied ";
#print STDERR ($source=~ s/__attribute/__attribute/g) || "no";
#print STDERR " patches.\n";

$alignment=0;
$max_align=0;

sub ins_packed_struct
{
	local ($name,$text)=@_;
	local ($return);

	$alignment=0;
	$max_align=0;

#       $text=~ s/(LONG|struct)([^;])/$1$2 __attribute__((aligned(2))) /g;

	$return="struct".$name.$text." __attribute__((packed));";

#FIXME: /* ; */ is not recogniced and 2-word types(eg. unsigned int) as well
#FIXED!?
	$return=~ s/^(\s*)(\w*)(\s*)([a-zA-Z0-9_]*)(.*?);/&ins_align($1,$2,$3,$4,$5)/ge;

	if($max_align>1 && $alignment>0 && ($alignment&1)!=0) {
	    $return=~ s/(.*)}(\s*__attribute__\(\(packed\)\).*)/$1\tchar __pad__;\n}$2/;
	}
	return $return;
}
sub ins_packed_typedef
{
	local ($text,$name)=@_;
	local ($return);


#       $text=~ s/(LONG|struct)([^;])/$1$2 __attribute__((aligned(2))) /g;

#       $return="struct".$name.$text." __attribute__((packed));";

#FIXME: /* ; */ is not recogniced and 2-word types(eg. unsigned int) as well
#FIXED!?
#       $return=~ s/^(\s*)(\w*)(\s*)([a-zA-Z_]*)(.*?);/&ins_align($1,$2,$3,$4,$5)/ge;

	return "typedef $name;";
}

sub ins_align
{
	local ($space,$type,$space2,$type2,$part1)=@_;
	local ($size,$x);
	$size=0;

	if ( $part1=~ /^\s*\*/ ) {
	    $size=4;
	    if ( $alignment!= 0 ) {
		if ( $part1=~ /^\s*(\**)(\w*)(.*)/ ) {
		    $part1=$1." __attribute__((aligned(2))) ".$2.$3;
		}
	    }
	} elsif( $type=~ /^(BYTE|UBYTE|BYTEBITS|TEXT|char)$/ ) {
	    $size=1;
	} elsif( $type=~ /^(WORD|UWORD|SHORT|USHORT|BOOL|COUNT|UCOUNT|WORDBITS|short)$/ ) {
	    if ($alignment&1) {
		$type=$type." __attribute__((aligned(2)))";
	    }
	    $size=2;
	} elsif( $type=~ /^struct$/ ) {
	    if ($alignment != 0) {
		$type2=$type2." __attribute__((aligned(2)))";
	    }
	    if( exists $oddstructs{$type2} ) {
		$size=$oddstructs{$type2};
	    } else {
		$size=4;
	    }
	} elsif( $alignment!=0 ) {
	    if( $type=~ /^([AC]PTR|STRPTR|LONG|LONGBITS|ULONG|FLOAT|DOUBLE|BPTR|BSTR)$/ ) {
		$type=$type." __attribute__((aligned(2)))";
		$size=4;
	    } elsif( $type=~ /^unsigned$/ ) {
		$type2=$type2." __attribute__((aligned(2)))";
		$size=4;
	    } elsif( $type=~ /^(int|long)$/ ) {
		$type=$type." __attribute__((aligned(2)))";
		$size=4;
	    } elsif( $part1=~ /^(\*|\(\*)/ ) {
		$type=$type." __attribute__((aligned(2)))";
		$size=4;
	    }
	}
	if( $alignment==1 && $size>1 ) {
	    $alignment=2;
	} elsif( $alignment==3 && $size>1) {
	    $alignment=0;
	}
	if( $size!=0 && $alignment!=-1 ) {
	    $x=$part1;
	    while ($alignment!=-1 && $x=~/\[/) {
		if( $x=~ /\[(\d*)\](.*)/ ) {
		    $size*=$1;
		    $x=$2;
		} else {
		    $alignment=-1;
		}
	    }
	    $x=$part1;
	    $x=~s/\[.*\]//g;
	    $x=~s/[^,]//g;
	    $size*=length($x)+1;
	    if( $alignment!=-1 ) {
		$alignment=($alignment+$size)&3;
	    }
	}
	if( $size > $max_align) {
	    $max_align = $size;
	}
#        return "/* ".$alignment.",".$max_align." */ ".$space.$type.$space2.$type2.$part1.";";
	return $space.$type.$space2.$type2.$part1.";";
}
