#!perl -w

#      for( $i=0; $i<50; $i++ ) {
#      $r = int(rand(4)-2);
#      print "$r\n";
#      }
#      exit;

# 0, 36, 73, 109, 146, 182, 219, 255
# 0, 85, 170, 255

@grbval2 = ( 0, 255 );
@grval4 = ( 0, 73, 182, 255 );
@bval4 = ( 0, 85, 170, 255 );
@grval8 = ( 0, 36, 73, 109, 146, 182, 219, 255 );

@gtab = ( 0, 255, 255,   0,   0, 255, 255,   0 );
@rtab = ( 0, 255,   0, 255,   0, 255,   0, 255 );
@btab = ( 0, 255,   0,   0, 255,   0, 255, 255 );

$num = 8;

print "4x2x2\n";
&inittemp;
for( $b=0; $b<2; $b++ ) {
  for( $r=0; $r<2; $r++ ) {
    for( $g=0; $g<4; $g++ ) {
      $exist = 0;
      $gv = $grval4[$g];
      $rv = $grbval2[$r];
      $bv = $grbval2[$b];
      &pushtemp;
    }
  }
}
&addtemp;

print "4x4x2\n";
&inittemp;
for( $b=0; $b<2; $b++ ) {
  for( $r=0; $r<4; $r++ ) {
    for( $g=0; $g<4; $g++ ) {
      $exist = 0;
      $gv = $grval4[$g];
      $rv = $grval4[$r];
      $bv = $grbval2[$b];
      &pushtemp;
    }
  }
}
&addtemp;

print "4x4x4\n";
&inittemp;
for( $b=0; $b<4; $b++ ) {
  for( $r=0; $r<4; $r++ ) {
    for( $g=0; $g<4; $g++ ) {
      $exist = 0;
      $gv = $grval4[$g];
      $rv = $grval4[$r];
      $bv = $bval4[$b];
      &pushtemp;
    }
  }
}
&addtemp;

print "8x4x4\n";
&inittemp;
for( $b=0; $b<4; $b++ ) {
  for( $r=0; $r<4; $r++ ) {
    for( $g=0; $g<8; $g++ ) {
      $exist = 0;
      $gv = $grval8[$g];
      $rv = $grval4[$r];
      $bv = $bval4[$b];
      &pushtemp;
    }
  }
}
&addtemp;

print "8x8x4\n";
&inittemp;
for( $b=0; $b<4; $b++ ) {
  for( $r=0; $r<8; $r++ ) {
    for( $g=0; $g<8; $g++ ) {
      $exist = 0;
      $gv = $grval8[$g];
      $rv = $grval8[$r];
      $bv = $bval4[$b];
      &pushtemp;
    }
  }
}
&addtemp;

&show;

print "Result mapping\n";
$resnum = 0;
@restab = ();
$row = 0;
for( $b=0; $b<4; $b++ ) {
  for( $r=0; $r<8; $r++ ) {
    for( $g=0; $g<8; $g++ ) {
      $gv = $grval8[$g];
      $rv = $grval8[$r];
      $bv = $bval4[$b];
      for( $i=0; $i<$num; $i++ ) {
        if( $gtab[$i] == $gv && $rtab[$i] == $rv && $btab[$i] == $bv ) {
          print "$i,";
          $row++;
          if( $row >=16 ) {
            $row = 0;
            print "\n";
          }
          push @restab, $i;
          $resnum++;
        }
      }
    }
  }
}
print "\nResnum: $resnum\n\n";

@gtab = ();
@rtab = ();
@btab = ();
$resnum = 0;
for( $b=0; $b<4; $b++ ) {
  for( $r=0; $r<8; $r++ ) {
    for( $g=0; $g<8; $g++ ) {
      $gtab[$restab[$resnum]] = $grval8[$g];
      $rtab[$restab[$resnum]] = $grval8[$r];
      $btab[$restab[$resnum]] = $bval4[$b];
      $resnum++;
    }
  }
}
#&show;
exit;


sub inittemp
{
  @gtemp = ();
  @rtemp = ();
  @btemp = ();
  $numtemp = 0;
}

sub pushtemp
{
#      print "$g $r $b -> $gv $rv $bv\n";
      for( $i=0; $i<$num; $i++ ) {
#        print "i $i\n";
#        print "g $gtab[$i]\n";
#        print "r $rtab[$i]\n";
#        print "b $btab[$i]\n";
        if( $gtab[$i] == $gv && $rtab[$i] == $rv && $btab[$i] == $bv ) {
          $exist = 1;
#          print "Found Tab $i: $gtab[$i], $rtab[$i], $btab[$i]\n";
        } else {
#          print "      Tab $i: $gtab[$i], $rtab[$i], $btab[$i]\n";
        }
      }
      if( !$exist ) {
#        print " New: $gv, $rv, $bv\n";
        push @gtemp, $gv;
        push @rtemp, $rv;
        push @btemp, $bv;
        $numtemp++;
      }
}

sub addtemp
{
      print "Numtemp $numtemp\n";
      @indx = ();
      for( $i=0; $i<$numtemp; $i++ ) {
        push @indx, $i;
      }
      @indx = sort { int(rand(4)-2) } @indx;
      for( $i=0; $i<$numtemp; $i++ ) {
#        print "-> $i $gtemp[$indx[$i]] $rtemp[$indx[$i]] $btemp[$indx[$i]]\n";
        push @gtab, $gtemp[$indx[$i]];
        push @rtab, $rtemp[$indx[$i]];
        push @btab, $btemp[$indx[$i]];
      }
      $num += $numtemp;
}


sub show
{
      for( $i=0; $i<$num; $i++ ) {
        printf( "%3d: R %3d G %3d B %3d\n", $i, $rtab[$i], $gtab[$i], $btab[$i]);
#        print "i $i ";
#        print "g $gtab[$i] ";
#        print "r $rtab[$i] ";
#        print "b $btab[$i]\n";
      }
      print "\nNum: $num\n";
}
