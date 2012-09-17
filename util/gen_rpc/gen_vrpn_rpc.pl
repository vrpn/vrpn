#! /bin/perl -w
#/*===3rdtech===
#  Copyright (c) 2000 by 3rdTech, Inc.
#  All Rights Reserved.
#
#  This file may not be distributed without the permission of 
#  3rdTech, Inc. 
#  ===3rdtech===*/
#use strict;

# Parse vrpn message definitions to a "list of lists" parse tree
# and then print them out, or pass to the next section.

# The grammar defined below should make it obvious what is legal -
# basically messages with a list of parameters. 

# Any variable length arrays should have their length included earlier
# in the parameter list (obviously), but this is not checked for. 

# A message can have only one structure included, and it must be
# the last parameter, and it must be an array (otherwise, you'd just 
# use a normal list of parameters, right?). 

# "pretty print" the parse tree, so the ARRAYs don't show. 
# Mostly borrowed from "demo_parsetree.pl" distributed with 
# the RecDescent module. 
sub printtree
{
  if (defined($_[1])) {
       if (!ref($_[1]) ) { 
          print "  " x $_[0];
          print "$_[1]:\n"; 
          foreach ( @_[2..$#_] )
          {
            if (ref($_)) { print "  " x $_[0], "$_\n"; printtree($_[0]+1,@$_); }
            else	     { print "  " x $_[0], "$_\n" }
          }
          print "\n";
        } else { 
          foreach ( @_[1..$#_] ) {
            #print ".";
            if (ref($_)) { print "  " x $_[0], "$_\n"; printtree($_[0],@$_); }
            else	     { print "  " x $_[0], "$_\n" }
          }
        }
      }
}

# Literal print of the parse tree, showing position of all ARRAYs
# Most useful for writing code to interpret the parse tree. 
sub printtree2
{
  if (defined($_[1])) {
    foreach ( @_[1..$#_] ) {
      #print ".";
      if (ref($_)) { print "  " x $_[0], "$_\n"; printtree2($_[0]+1,@$_); }
      else	     { print "  " x $_[0], "$_\n" }
    }
  }
}
  
# Package that produces the parse tree. 
# At UNC, perl in /contrib/bin has this package installed.
# Otherwise, go to www.cpan.org and download it.
# unpack, run "perl Makefile.pl" "make" "make install" to install it. 
use Parse::RecDescent;

#$::RD_HINT = 1;     # turn on hints about problems/errors.
#$::RD_TRACE = 1;     # turn on very verbose debugging

# This rule forms a "list of lists" parse tree automatically from
# the grammar. 
# One funky thing - any rule with a (?), (s) or (s?) returns an array ref,
# which has a member for each time the rule matched. If it didn't match at
# all, the array is empty. 
$RD_AUTOACTION = q{ [@item] };

$grammar =
q{
  start: msg_group msg_prefix(?) defines(s?) line(s)

  msg_group: /MESSAGE_GROUP/ /(\w)+/

  # Prefix is used for the identifying string when registering
  # a message type with VRPN. For compatibility with legacy
  # stream files, mostly. 
  msg_prefix: /MESSAGE_PREFIX/ /".*"/

  # Define is for fixed counts used in message definitions.
  # No checks are made later to see if a fixed count is defined, though. 
  defines: /DEFINE/ /[A-Z][A-Z0-9_]+/ /[^\n]*\n/

  line: /VRPN_MESSAGE/ typeName /{/ structOrField /}/
#    | /[^\n]*\n/

  typeName: /(\w)+/

  structOrField:
    field(s) structDefinition
    | field(s?)

  structDefinition:
      /struct/ structTypeName /{/
        field(s)
      /}/ structName count
          # Note that count is required - must be an array. 

  structName:
      /(\w)+/

  structTypeName:
      /(\w)+/

  field:
      /(\w)+/ /(\w)+/ fixedCount(s?) variableCount(s?) <reject: $item[1] eq "struct">
  # Reject directive is necessary, otherwise field rule will 
  # gobble "struct typename" and leave { ... } 

  count:
    fixedCount
    | variableCount

  # One kind is not allowed - mixed case that starts with a CAP
  fixedCount:
      /\[/ /(\d)+/ /\]/   # fixed, numeric count
    | /\[/ /[A-Z][A-Z0-9_]+/ /\]/ # fixed, all caps and "_" #defined count

  variableCount:
    /\[/ /[a-z](\w)+/ /\]/ # variable count, starts with lower case. 
  
};
$definitionParser = $definitionParser;
$definitionParser = new Parse::RecDescent ($grammar);
#$definitionInput = "";
# Get rid of the autoaction so we don't interfere with other grammars
$RD_AUTOACTION = "";

# Parse tree is not generated until requested by second grammar below. 

# VERY USEFUL view of the parse tree, shows how the grammar
# above is made into a parse tree for a particular set of messages. 
#printtree2(0,@$tree) if $tree;

sub parseMsgDefFile ($)
{
  my $msg_def_file = $_[0];
  if ($msg_def_file !~ /.*\.vrpndef$/) {
    $msg_def_file .= ".vrpndef";
  }
  my $definitionInput = "";
  my $line = "";
  print "//Message definitions from file " . $msg_def_file . "\n";
  open(DEFHANDLE, "< " . $msg_def_file ) || 
      die "Couldn't open vrpn msg definition file ". $msg_def_file ;
  while (defined($line = <DEFHANDLE>)) {
    #chomp $line;
    $line =~ s#//.*$##;          # remove comments
    $definitionInput .= $line if $line !~ /^\s*$/;  # return if not white-space
  } 
  close(DEFHANDLE);
  
  # Remove /* */ comments - non-greedy "*?" regexp, 
  # global replace "g" on the string including newlines "s".
  $definitionInput =~ s#/\*.*?\*/##gs;
  
  my $tree = $::definitionParser->start($definitionInput);
  if (!$tree) { 
    die "Unable to parse vrpn rpc msg definition file ". $msg_def_file ;
  }
  
  # VERY USEFUL view of the parse tree, shows how the grammar
  # above is made into a parse tree for a particular set of messages. 
  #printtree2(0,@$tree) if $tree;

  # Get a reference to the list of messages from the parse tree
  my $tmp_msgs = @$tree[4];
  # Get a reference to the (possibly empty) list of defines, too. 
  my $tmp_defines = @$tree[3];
  # Get a reference to the (possibly empty) msg prefix, too. 
  my $tmp_msg_prefix = "";
  if (defined(@$tree[2]->[0])) {
    $tmp_msg_prefix = @$tree[2]->[0]->[2];
    $tmp_msg_prefix =~ tr/\"//d;
    #print STDOUT "Prefix found: $tmp_msg_prefix \n";
  }

  # Global hash to contain msg group names and the parse tree generated. 
  $::msg_trees{@$tree[1]->[2]} = $tmp_msgs;
  $::msg_trees{@$tree[1]->[2] . "_defines"} = $tmp_defines;
  $::msg_trees{@$tree[1]->[2] . "_msg_prefix"} = $tmp_msg_prefix;
  $::most_recent_tree = @$tree[1]->[2];
}

#--------------------------------------------------------------
# This section uses the parse tree generated above to 
# output various C++ function declarations and definitions. 

# If set, function declarations and definitions are C++ style using
# the specified className. 
$className = "";
$classPrefix = "";

# If headerFlag is 1, emit functions will only emit declarations
$headerFlag = 0;


# Emit the parameter list, for encoder, decoder, receiver fcns. 
sub emitParamList ($;$$) {
  my $fieldarray = $_[0];
  my $varPrefix = $_[1];
  my $pointerFlag = 0;
  if ($_[2]) { $pointerFlag = $_[2] };
  foreach $thisarg (@$fieldarray) {
    print "      ";
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];

    if (($vartype ne "char") && ($vartype ne "timeval")) {
      print "vrpn_";
    }
    # print the type. 
    print $vartype . " " ;

    if ($pointerFlag) {
      # Print a "*" for each dimension of an array. 
      if (defined(@$thisarg[3]->[0])||
          defined(@$thisarg[4]->[0])) {
        print "*" x ($#{@$thisarg[4]} + $#{@$thisarg[3]} + 2);
      }
    } else {
      # Find out if we have a variable array
      if (defined(@$thisarg[4]->[0])) {
        # print one star for each dimension
        print "*" x ($#{@$thisarg[4]} +1);
      } 
    }
    print "". ($varPrefix ? "(":"") . $varPrefix . $varname 
        . ($varPrefix ? ")":"") ;
    if (!$pointerFlag) {
      # NOTE: for some reason these declarations cause problems. 
      # C++ compiler complains about passing in an array instead of pointer.
      # Find out if we have a fixed array
      if (defined(@$thisarg[3]->[0])) {
        # print the fixed array spec
        foreach $dim (@{@$thisarg[3]}) {
          print @$dim[1..3];
        }
      }
    }
    # Skip comma on last parameter
    if ($thisarg != @$fieldarray[$#$fieldarray]) {
      print ",";
    }

    print "\n";
  }
  # On to the special case - an array has a variable count which
  # is not specified in one of the other args - it must be passed
  # in separately. So add it to the param list. 
  foreach $thisarg (@$fieldarray) {
    # Check for variable count(s)
    if (defined(@$thisarg[4]->[0])) {
      foreach (@{@$thisarg[4]}) {
        my $countvar = @$_[2];
#        print STDOUT $countvar . " ";
        my $foundit = 0;
        foreach $morearg (@$fieldarray) {
#          print STDOUT @$morearg[2] . " ";
          $foundit = 1 if ($countvar eq @$morearg[2]);
        }
#        print STDOUT "\n";
        # If we didn't find it, add it to the arg list. 
        # NOTE force to vrpn_int32 - is this the right thing?
        if (!$foundit) {
          print "      , vrpn_int32 " . ($varPrefix ? "(":"") 
              . $varPrefix . $countvar . ($varPrefix ? ")":"") ."\n" ;
        }
      }
    }
  }
        
}

# Emits a function to decode the named type into network form.
# Returns 0, or -1 on error.
#
#  int myClass::decode_foo (const char * buffer, ...) {
#    if (!buffer || ...) return -1;
#    vrpn_unbuffer(&buffer, ...);
#    return 0;
#  }
# However, a message that includes a structure needs two decoders,
# one for the simple parameters, and another to be called in a loop
# to decode the structure parameters. 
sub emitDecoders ($) {
  my $msgarray = $_[0];
  # Array contains (line VRPN_MESSAGE (typeName InContactMode) { ... }
  # Extract the value of typeName and the arg list array (...)
  my $typeName = @$msgarray[2]->[1];
  my $fieldarray = @$msgarray[4]->[1];
  my $structarray = []; my $structfieldarray = [];
  # If this message contains a struct array, do more complicated things. 
  if (defined(@$msgarray[4]->[2])) { 
    $structarray = @$msgarray[4]->[2];
    $structfieldarray = @$structarray[4];
  }
  
  # Structure requires two decode functions, for header and struct body. 
  if ($#$structarray >= 0) {
    if ( $#$fieldarray <0) {
      # We have a struct with no fields before it -
      # the struct must be repeated a const number of times
      # in the message. Just emit the decode_X_body
      emitDecoderFromFields($structfieldarray, $typeName , "_body");
    } else {
      # Emit decoder for both header and struct body.
      # handler will call _body from inside appropriate loop.
      emitDecoderFromFields($fieldarray, $typeName , "_header");
      emitDecoderFromFields($structfieldarray, $typeName , "_body");
    }
    return 1;
  } 
  # If the message has no fields, no decode function is needed. 
  if ( $#$fieldarray <0) { 
    print "// decode_$typeName not needed - empty message.\n";
    return 1;
  }
  # Emit decoder for a message that doesn't contain a structure. 
  emitDecoderFromFields($fieldarray, $typeName , "");
  1;
};

# Emits a decoder from an array of fields and a typename. Appends
# the namePostfix to the end of the function name.
sub emitDecoderFromFields ($$;$) {
  my $fieldarray = $_[0];
  my $typeName = $_[1];
  my $namePostfix = "";
  if ($_[2]) { $namePostfix = $_[2]; }

  # Emit function declaration / parameter list
  if ($headerFlag) { print "  "; }
  print "int " . ($headerFlag ? "" : $classPrefix) 
      . "decode_${typeName}${namePostfix} (\n";
  print "      const char ** buffer,\n";

  # Loop over parameter list
  emitParamList($fieldarray, "*", 0);

  if ($headerFlag) { print "  "; }
  print ")";
  # If this is just a declaration, we are done. 
  if ($headerFlag) { print ";\n"; return 1; }
  print " {\n";

  # Emit function body

  # Emit test for non-NULL
  print "  if (!buffer";
  foreach $thisarg (@$fieldarray) {
    my $varname = @$thisarg[2];
    print "\n   || !" . $varname;
  }
  print ") return -1;\n";

  # OK, tough stuff:  unbuffer each entry.
  foreach $thisarg (@$fieldarray) {
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];
    
    my $vardim = $#{@$thisarg[3]} +1 + $#{@$thisarg[4]} +1;
    if ($vartype eq "char") { $vardim -= 1; }

    if ($vardim > 0) {
	# We have an array. 
	my $prefix = "  ";
	my $dimcount = $vardim;
        # appending lists like this means we do fixed arrays first. 
        my @countlist = ( @{@$thisarg[3]} , @{@$thisarg[4]});
        my $charcount;
        if ($vartype eq "char") { 
          # Last array dimension handled by "vrpn_unbuffer" for char
          $charcount = $countlist[$#countlist];
          @countlist = @countlist[0..($#countlist-1)];
        }
        #print @countlist, "\n";
	foreach ( @countlist) {
	    my $countvar = @$_[2];
            # variable count arrays require memory allocation. 
            if (@$_[0] eq "variableCount") {
              # De-reference the parameter passed in. 
              print $prefix . "(*$varname)";
	      for ($i = $vardim; $i >$dimcount ; $i--) {
		  print "[lv_$i]";
	      }
	      print " = new " . ((($vartype eq "char") || ($vartype eq "timeval")) ? "" : "vrpn_") 
                  . $vartype . " ";
              print "*" x ($vartype eq "char" ? $dimcount : $dimcount -1);
              print " [".(@$_[0] eq "variableCount" ? "*":"") . @$_[2] . "];\n";
            }
	    print $prefix . "for (int lv_${dimcount} = 0" .
		"; lv_${dimcount} < ".
                    (@$_[0] eq "variableCount" ? "*":"")."$countvar ".
		"; lv_${dimcount}++ ) {\n";
	    $dimcount -= 1;
            $prefix = $prefix . "  ";
	}
        if ($vartype eq "char") {
          # If a variable length character string, allocate space. 
          if (@$charcount[0] eq "variableCount" ) {
	    print  $prefix . "(*$varname)";
            for ($i = $vardim; $i >0 ; $i--) {
              print "[lv_$i]";
            }
            print " = new char[" .(@$charcount[0] eq "variableCount" ? "*":"")
                . @$charcount[2] . "];\n";
          }
        }
	print $prefix . "if (vrpn_unbuffer(buffer, ";
	if ($vartype ne "char") {
	    print "&(";
	}
	print "(*$varname)";
        for ($i = $vardim; $i >0 ; $i--) {
          print "[lv_$i]";
        }
	if ($vartype ne "char") {
	    print ")";
	}
        if ($vartype eq "char") {
          print ", " .(@$charcount[0] eq "variableCount" ? "*":"") 
              . @$charcount[2] ;
        }
        print ")) return -1;\n";
        for ($i = $vardim; $i >0 ; $i--) {
          print "  " x $i;
          print "}\n";
        }
	
    } else {
	# Normal variable, might be character array. 
      if ($vartype eq "char") {
        # If a variable length character string, allocate space. 
        if (defined(@$thisarg[4]->[0])) {
	    print "  *" . $varname . " = new char["
                . (@$thisarg[4]->[0]->[0] eq "variableCount" ? "*":"")
                . @$thisarg[4]->[0]->[2] . "];\n";
	}
      }
      print "  if (vrpn_unbuffer(buffer, ";
      if ($vartype eq "char") {
	  print "*" ;
      }
      print  $varname;
      if ($vartype eq "char") {
	print ", " ;
	my $vardim = 0;
	# Print fixed or variable array count
	if (defined(@$thisarg[3]->[0])) {
	    print "" . (@$thisarg[3]->[0]->[0] eq "variableCount" ? "*":"")
                . @$thisarg[3]->[0]->[2];
	} elsif (defined(@$thisarg[4]->[0])) {
	    print "" . (@$thisarg[4]->[0]->[0] eq "variableCount" ? "*":"")
                . @$thisarg[4]->[0]->[2];
	}
      }
      print ")) return -1;\n";
    }
  }

  print "  return 0;\n";
  print "}\n";
  print "\n";

  1;
};

# Given a field list, emit a sum of sizeof() calls. 
# Handle multi-dimensional arrays, too. 
sub emitFieldSizes ($)
{
  my $fieldarray = $_[0];
  print "(";
  foreach $thisarg (@$fieldarray) {
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];
    my $vardim = $#{@$thisarg[3]} +1 + $#{@$thisarg[4]} +1;

    print "sizeof(" . ((($vartype eq "char") || ($vartype eq "timeval")) ? "" : "vrpn_") . $vartype . ")";
    if ($vardim > 0) {
      my @countlist = ( @{@$thisarg[3]} , @{@$thisarg[4]});
      foreach ( @countlist) {
        my $countvar = @$_[2];
        print " * " . $countvar;
      }
    }
    if ($thisarg != @$fieldarray[$#$fieldarray]) {
      print " + ";
    }
  }
  print ")";
  1;
};

# Given a list of fields, emit a "vrpn_buffer" call for each one.
# Handle arrays by emitting a "for" loop to buffer each element. 
sub emitBufferList ($)
{
  my $fieldarray = $_[0];
  foreach $thisarg (@$fieldarray) {
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];
    
    my $vardim = $#{@$thisarg[3]} +1 + $#{@$thisarg[4]} +1;
    if ($vartype eq "char") { $vardim -= 1; }

    if ($vardim > 0) {
	# We have an array. 
	my $prefix = "    ";
	my $dimcount = $vardim;
        # appending lists like this means we do fixed arrays first. 
        my @countlist = ( @{@$thisarg[3]} , @{@$thisarg[4]});
        my $charcount;
        if ($vartype eq "char") { 
          # Last array dimension handled by "vrpn_buffer" for char
          $charcount = $countlist[$#countlist];
          @countlist = @countlist[0..($#countlist-1)];
        }
        #print @countlist, "\n";
	foreach ( @countlist) {
	    my $countvar = @$_[2];
	    print $prefix . "for (int lv_${dimcount} = 0" .
		"; lv_${dimcount} < $countvar" .
		"; lv_${dimcount}++ ) {\n";
	    $dimcount -= 1;
            $prefix = $prefix . "  ";
	}
	print $prefix . "vrpn_buffer(mptr, mlen, ";
	print $varname;
        for ($i = $vardim; $i >0 ; $i--) {
          print "[lv_" . $i . "]";
        }
        if ($vartype eq "char") {
          print ", " . @$charcount[2] ;
        }
        print ");\n";
        for ($i = $vardim; $i >0 ; $i--) {
          print "  " x ($i+1);
          print "}\n";
        }
	
    } else {
      # Normal variable, might be character array. 
      print "    vrpn_buffer(mptr, mlen, ";
      print  $varname;
      if ($vartype eq "char") {
	print ", " ;
	# Print fixed or variable array count
	if (defined(@$thisarg[3]->[0])) {
	    print @$thisarg[3]->[0]->[2];
	} elsif (defined(@$thisarg[4]->[0])) {
	    print @$thisarg[4]->[0]->[2];
	}
      }
      print ");\n";
    }
  }
  1;
};

# Emits a function to encode the named type into network form.
# Returns a buffer which it is the caller's responsibility to
# delete [].  Places the length of the buffer in the parameter
# len.
#
#  char * myClass::encode_foo (int * len, ...) {
#    char * msgbuf = NULL;
#    char * mptr;
#    int mlen;
#    if (!len) return NULL;
#    *len = sizeof(...);
#    msgbuf = new char [*len];
#    if (!msgbuf) {
#      fprintf(stderr, "encode_foo:  Out of memory.\n");
#      *len = 0;
#    } else {
#      mptr = msgbuf;
#      mlen = *len;
#      vrpn_buffer(&mptr, &mlen, ...);
#    }
#    return msgbuf;
#  }
# However, a message that includes a structure needs two encoders,
# one for the simple parameters, and another to be called in a loop
# to encode the structure parameters. 
sub emitEncoders ($) {
  my $msgarray = $_[0];
  # Array contains (line VRPN_MESSAGE (typeName InContactMode) { ... }
  # Extract the value of typeName and the arg list array (...)
  my $typeName = @$msgarray[2]->[1];
  my $fieldarray = @$msgarray[4]->[1];
  my $structarray = []; my $structfieldarray = [];
  # If this message contains a struct array, do more complicated things. 
  if (defined(@$msgarray[4]->[2])) { 
    $structarray = @$msgarray[4]->[2];
    $structfieldarray = @$structarray[4];
  }
  # Structure requires two encode functions, for header and struct body. 
  if ($#$structarray >= 0) {
    if ( $#$fieldarray <0) {
      # We have a struct with no fields before it -
      # the struct must be repeated a const number of times
      # in the message. Just emit the encode_X_body
      emitBodyEncoderFromFields($structfieldarray, $typeName, "_body");
    } else {
      # Emit encoder for both header and struct body.
      # handler will call _body from inside appropriate loop.
      emitEncoderFromFields($fieldarray, $typeName, 
                            $structarray, "_header");
      emitBodyEncoderFromFields($structfieldarray, $typeName, "_body");
    }
    return 1;
  } 
  # If the message has no fields, no encode function is needed. 
  if ( $#$fieldarray <0) { 
    print "// encode_$typeName not needed - empty message.\n";
    return 1;
  }
  # Emit encoder for a message that doesn't contain a structure. 
  emitEncoderFromFields($fieldarray, $typeName , $structfieldarray, "");
  1;
};

# Emits a encoder from an array of fields and a typename. Appends
# the optional namePostfix to the end of the function name. 
# If the structure array parameter is not empty, emit the 
# encoder for the non-struct parameters in the message, 
# and include extra parameters so the incomplete encoding can be tracked. 
sub emitEncoderFromFields ($$$;$) {
  my $fieldarray = $_[0];
  my $typeName = $_[1];
  my $structarray = $_[2]; my $structfieldarray = [];
  my $structCount = 0;
  my $namePostfix = $_[3];
  if ($#$structarray >= 0) {
    $structfieldarray = @$structarray[4];
    $structCount = @$structarray[7]->[1]->[2];
  }

  # function declaration / parameter list

  if ($headerFlag) { print "  "; }
  print "char * " . ($headerFlag ? "" : $classPrefix) 
      . "encode_${typeName}${namePostfix} (\n";
  print "      int * len,\n";
  # Return vars with our encoding progress if there's a struct. 
  if ($#$structarray >= 0) {
    print "      char ** mptr, int* mlen,\n";
  }
  emitParamList($fieldarray, "", 0);

  if ($headerFlag) { print "  "; }
  print ")";
  # If this is just a declaration, we are done. 
  if ($headerFlag) { print ";\n"; return 1; }
  print " {\n";

  # Emit function body

  print "  char * msgbuf = NULL;\n";
  if ($#$structarray < 0) {
    print "  char *mptr[1];\n";
    print "  int temp; int* mlen = &temp;\n";
  }
  print "  if (!len) return NULL;\n";

  # figure out size of message by adding up types of every entry.

  print "  *len =\n     ";
  emitFieldSizes($fieldarray);
  if ($#$structarray >= 0) {
    print "\n+ ";
    emitFieldSizes($structfieldarray);
    print " * " . $structCount ;
  }
  print ";\n";

  print "  msgbuf = new char [*len];\n"
      . "  if (!msgbuf) {\n"
      . "    fprintf(stderr, \"encode_${typeName}${namePostfix}: " .
                     "Out of memory.\\n\");\n"
      . "    *len = 0;\n"
      . "  } else {\n"
      . "    *mptr = msgbuf;\n"
      . "    *mlen = *len;\n";

  # OK, tough stuff:  buffer each entry.
  # Need to handle nested loops!

  emitBufferList($fieldarray);

  print "  }\n";
  print "  return msgbuf;\n";
  print "}\n";
  print "\n";

  1;
};

# Emits a encoder for a structure body from an array of fields and a
# typename. Appends the optional namePostfix to the end of the
# function name.
# Pass in the existing buffer, progress pointer, and length. 
# This function keeps track of encoding progress by returning a 
# char pointer and length in extra parameters. 
sub emitBodyEncoderFromFields ($$;$) {
  my $fieldarray = $_[0];
  my $typeName = $_[1];
  my $namePostfix = $_[2];

  # function declaration / parameter list
  if ($headerFlag) { print "  "; }
  print "char * " . ($headerFlag ? "" : $classPrefix) 
      . "encode_${typeName}${namePostfix} (\n";
  print "      int * len, char * msgbuf, char ** mptr, int *mlen, \n";

  emitParamList($fieldarray, "", 0);

  if ($headerFlag) { print "  "; }
  print ")";
  # If this is just a declaration, we are done. 
  if ($headerFlag) { print ";\n"; return 1; }
  print " {\n";

  # Emit function body

  print "  if (!len || !msgbuf || !mptr || !mlen) return NULL;\n";

  # OK, tough stuff:  buffer each entry.
  emitBufferList($fieldarray);

  print "  return msgbuf;\n";
  print "}\n";
  print "\n";

  1;
};

# Variable declarations for handler. 
sub emitVarDecl ($) {
    my $thisarg = $_[0];

    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];

    if (($vartype ne "char") && ($vartype ne "timeval")) {
      print "vrpn_";
    }
    # print the type. 
    print $vartype . " ";
    # Find out if we have a variable array
    if (defined(@$thisarg[4]->[0])) {
      # print one star for each dimension
      print "*" x ($#{@$thisarg[4]} +1);
    }
    print $varname;
    # Find out if we have a fixed array
    if (defined(@$thisarg[3]->[0])) {
      # print the fixed array spec
      foreach $dim (@{@$thisarg[3]}) {
        print @$dim[1..3];
      }
    }

    print ";\n";


}

# Emit an arg list for calling an encode, decode or receive fcn. 
sub emitArgList ($;$) 
{
  my $fieldarray = $_[0];
  my $varPrefix = "";
  if ($_[1]) { $varPrefix = $_[1]; }
    foreach $thisarg (@$fieldarray) {
      my $vartype = @$thisarg[1];
      my $varname = @$thisarg[2];
      # Array params DON'T need cast. 
#      if (defined(@$thisarg[3]->[0]) || 
#          defined(@$thisarg[4]->[0])) {
#        print "(" .((($vartype eq "char") || ($vartype eq "timeval")) ? "" : "vrpn_") . "$vartype ";
#        # One star for each dimension, plus one extra. 
#        print "*" x ($#{@$thisarg[4]} + $#{@$thisarg[3]} + 3);
#        print ")";
#      }
      print $varPrefix . $varname ;
      if ($thisarg != @$fieldarray[$#$fieldarray]) {
        print ", ";
      }
    }
    # On to the special case - an array has a variable count which
    # is not specified in one of the other args - it must be passed
    # in separately. So add it to the arg list. 
    foreach $thisarg (@$fieldarray) {
      # Check for variable count(s)
      if (defined(@$thisarg[4]->[0])) {
        foreach (@{@$thisarg[4]}) {
          my $countvar = @$_[2];
          my $foundit = 0;
          foreach $morearg (@$fieldarray) {
            $foundit = 1 if ($countvar eq @$morearg[2]);
          }
          # If we didn't find it, add it to the arg list. 
          if (!$foundit) {
            print ", " . $varPrefix . $countvar ;
          }
        }
      }
    }
}

# Emit a call to decode_TYPE, then rcv_TYPE, for a particular message
# namePostix modifies the function names so they are right for structs. 
# prefix is spacing for pretty-printing. 
sub emitDecodeRcvCall ($$$$)
{
  my $fieldarray = $_[0];
  my $typeName = $_[1];
  my $namePostfix = $_[2];
  my $prefix = $_[3];
  # Call decode/Rcv for simple arg lists and first part of struct msg 
  # Only decode if there are args to decode. 
  if (defined(@$fieldarray[0])) {
    print $prefix. "decode_${typeName}${namePostfix} ( &buffer, ";
    emitArgList($fieldarray, "&");
    print " );\n";
  }
  
  print $prefix. "Rcv${typeName}${namePostfix} ( ";
  emitArgList($fieldarray);
  print " );\n";
  # Delete allocated memory for var length arrays. 
  foreach $thisarg (@$fieldarray) {
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];
    # Variable arrays get dynamic memory deleted. 
    if (defined(@$thisarg[4]->[0])) {
        my $vardim = $#{@$thisarg[3]} +1 + $#{@$thisarg[4]} +1;
	my $prefix = "  ";
        # appending lists like this means we do fixed arrays first. 
        my @countlist = ( @{@$thisarg[3]} , @{@$thisarg[4]});
        # Last array dimension handled by delete []
        @countlist = @countlist[0..($#countlist-1)];
        $vardim -= 1;
 	my $dimcount = $vardim;
	foreach ( @countlist) {
	    my $countvar = @$_[2];
	    print $prefix . "for (int lv_${dimcount} = 0" .
		"; lv_${dimcount} < $countvar ".
		"; lv_${dimcount}++ ) {\n";
	    $dimcount -= 1;
            $prefix = $prefix . "  ";
	}
        print $prefix . "delete [] " . $varname;
        for ($i = $vardim; $i >0 ; $i--) {
          print "[lv_$i]";
        }
        print ";\n";
        
        $dimcount = 1;
	foreach ( reverse @countlist) {
          # Chop two spaces off prefix. 
          $prefix =~ s/(.*)  $/$1/;
          print $prefix . "}\n";
          if (@$_[0] eq "variableCount") {
            print $prefix . "delete [] " . $varname;
            for ($i = $vardim; $i >$dimcount ; $i--) {
              print "[lv_$i]";
            }
            print ";\n";
          }
          $dimcount += 1;
        }
     
    }
  }
  
}

# Emits a function that unmarshalls (see decode_foo) onto the stack
# then call a Rcv function. Rcv function will contain user code. 
# Simple case looks roughly like this:
#//static
#int nmm_Microscope_Remote::handle_InScanlineMode (void *userdata, 
#					vrpn_HANDLERPARAM param) {
#  nmm_Microscope_Remote * ms = (nmm_Microscope_Remote *) userdata;
#  vrpn_int32 enabled;
#  
#  ms->decode_InScanlineMode(&param.buffer, &enabled);
#  ms->RcvInScanlineMode(enabled);
#
#  return 0;
#}

# Structure case adds a loop to the calls: 
#  me->decode_ReportScanDatasets_header( &count );
#  me->RcvReportScanDatasets_header( count );
#  for (int lv_1 = 0; lv_1 < count; lv_1++) {
#    me->decode_ReportScanDatasets_body( &name, &units, &offset, &scale );
#    me->RcvReportScanDatasets_body( name, units, offset, scale );
#  }

sub emitHandler ($) {
  my $msgarray = $_[0];
  # Array contains (line VRPN_MESSAGE (typeName InContactMode) { ... }
  # Extract the value of typeName and the arg list array (...)
  my $typeName = @$msgarray[2]->[1];
  my $fieldarray = @$msgarray[4]->[1];
  my $structarray = []; my $structfieldarray = [];
  # If this message contains a struct array, do more complicated things. 
  if (defined(@$msgarray[4]->[2])) { 
    $structarray = @$msgarray[4]->[2];
    $structfieldarray = @$structarray[4];
  }
  # Emit function declaration / parameter list

  
  print "" . ($headerFlag ? "  static ": "\n");
  print "int " . ($headerFlag ? "VRPN_CALLBACK " : $classPrefix) 
      . "handle_${typeName} (void * userdata, vrpn_HANDLERPARAM grpc_p)";

  # If this is just a declaration, we are done. 
  if ($headerFlag) { print ";\n"; return 1; }

  print " {\n";
  if (($#$fieldarray >= 0)||($#$structarray >= 0)) {
    print "  const char * buffer = grpc_p.buffer;\n";
  }
  if ((defined($className)) && ($className ne "")) {
    print "  " . $className . "* grpc_me = (" . $className . " *) userdata;\n"; 
  }
  foreach $thisarg (@$fieldarray) {
    print "  ";
    emitVarDecl( $thisarg );
  }
  if ($#$structarray >= 0) {
    foreach $structarg ( @$structfieldarray) {
      print "  ";
      emitVarDecl( $structarg );
    }
  } 

  print "\n";

  # Emit function body

  my $prefix = "  ";
  if ((defined($className)) && ($className ne "")) {
    $prefix .= "grpc_me->";
  }
  
  if ($#$structarray < 0) {
    emitDecodeRcvCall ($fieldarray, $typeName, "", $prefix);
  } else {
    emitDecodeRcvCall ($fieldarray, $typeName, "_header", $prefix);
    # Call decode_header/Rcv_header loop { decode_body/Rcv_body }
    # for messages that end in structure arrays. 
      print "  for (int lv_1 = 0; lv_1 < " . 
          @$structarray[7]->[1]->[2] . "; lv_1++) {\n";
    emitDecodeRcvCall ($structfieldarray, $typeName, "_body", "  " . $prefix);
    print "  }\n";
  }

  print "  return 0;\n";
  print "}\n";

  1;
};

# Emits a function to receive the msg type 
# and either print all the fields, or expect user code to follow.  
#
#  void myClass::receive_foo (vrpn_int32 bar, ...) {
# OR 
#  void myClass::receive_foo (vrpn_int32 bar, ...) {
#    print ("msg foo, bar = %d, ...", bar, ...);
#  }
# However, a message that includes a structure needs two receivers,
# one for the simple parameters, and another to be called in a loop
# to receive the structure parameters. 
sub emitReceivers ($;$$) {
  my $msgarray = $_[0];
  my $namePostfix = "";
  if ($_[1]) { $namePostfix = $_[1]; }
  my $printParams = 1; 
  if (defined($_[2])) { $printParams = $_[2]; }
  # Array contains (line VRPN_MESSAGE (typeName InContactMode) { ... }
  # Extract the value of typeName and the arg list array (...)
  my $typeName = @$msgarray[2]->[1];
  my $fieldarray = @$msgarray[4]->[1];
  my $structarray = []; my $structfieldarray = [];
  # If this message contains a struct array, do more complicated things. 
  if (defined(@$msgarray[4]->[2])) { 
    $structarray = @$msgarray[4]->[2];
    $structfieldarray = @$structarray[4];
  }
  # Structure requires two receive functions, for header and struct body. 
  if ($#$structarray >= 0) {
    # Figure out which to emit from
    if($namePostfix eq "_body") {
      emitReceiverFromFields($structfieldarray, $typeName, "_body", 1, $printParams);
    } elsif($namePostfix eq "_header") {
      emitReceiverFromFields($fieldarray, $typeName, "_header", 0, $printParams);
    } else {
      if ( $#$fieldarray <0) {
        # We have a struct with no fields before it -
        # the struct must be repeated a const number of times
        # in the message. Just emit the receive_X_body
        emitReceiverFromFields($structfieldarray, $typeName, "_body", 1, $printParams);
      } else {
        print "Probable error, two incomplete rcv fcns emitted\n" if (!$printParams) ;
        # Emit receiver for both header and struct body.
        # handler will call _body from inside appropriate loop.
        emitReceiverFromFields($fieldarray, $typeName, "_header", 0, $printParams);
        emitReceiverFromFields($structfieldarray, $typeName, "_body", 1, $printParams);
      }
    }
    return 1;
  } 
  # Emit receiver for a message that doesn't contain a structure. 
  emitReceiverFromFields($fieldarray, $typeName , "", 0, $printParams);
  1;
};

# Emits a receiver from an array of fields and a typename. Appends
# the namePostfix to the end of the function name.
sub emitReceiverFromFields ($$;$$$) {
  my $fieldarray = $_[0];
  my $typeName = $_[1];
  my $namePostfix = "";
  if ($_[2]) { $namePostfix = $_[2]; }
  my $structFlag = 0;
  if ($_[3]) { $structFlag = $_[3]; }
  my $printParams = 1;
  if (defined($_[4])) { $printParams = $_[4]; }
  
  # Emit function declaration / parameter list
  if ($headerFlag) { print "  "; }
  print "void " . ($headerFlag ? "" : $classPrefix) 
      . "Rcv${typeName}${namePostfix} (\n";

  # Loop over parameter list
  emitParamList($fieldarray, "", 0);

  if ($headerFlag) { print "  "; }
  print ")";
  # If this is just a declaration, we are done. 
  if ($headerFlag) { print ";\n"; return 1; }
  print " {\n";

  # If we are not going to print the parameters, we are done - user
  # code must follow
  if (!$printParams) { return 1; }

  # Otherwise, Emit function body

  if ($structFlag) {
    print "  printf(\"  struct $typeName: \"); \n";
  } else {
    print "  printf(\"MSG $typeName: \"); \n";
  }
  foreach $thisarg (@$fieldarray) {
    my $vartype = @$thisarg[1];
    my $varname = @$thisarg[2];
    
    my $vardim = $#{@$thisarg[3]} +1 + $#{@$thisarg[4]} +1;
    if ($vartype eq "char") { $vardim -= 1; }

    if ($vardim > 0) {
	# We have an array. 
	my $prefix = "  ";
	my $dimcount = $vardim;
        # appending lists like this means we do fixed arrays first. 
        my @countlist = ( @{@$thisarg[3]} , @{@$thisarg[4]});
        my $charcount;
        if ($vartype eq "char") { 
          # Last array dimension handled by "vrpn_unbuffer" for char
          $charcount = $countlist[$#countlist];
          @countlist = @countlist[0..($#countlist-1)];
        }
        #print @countlist, "\n";
        print "  printf(\"" . $varname . " = \");\n";
	foreach ( @countlist) {
            print $prefix . "printf(\"(\");\n";
	    my $countvar = @$_[2];
	    print $prefix . "for (int lv_${dimcount} = 0" .
		"; lv_${dimcount} < $countvar" .
		"; lv_${dimcount}++ ) {\n";
	    $dimcount -= 1;
            $prefix = $prefix . "  ";
	}
        print $prefix . "  printf(\"";
        if ($vartype eq "char") {
	  print "%s" ;
        } elsif ($vartype eq "int32") {
	  print "%d" ;
        } elsif ($vartype eq "float32") {
	  print "%g" ;
        } 
        print ", \", " . $varname;
        for ($i = $vardim; $i >0 ; $i--) {
          print "[lv_" . $i . "]";
        }
        print  " );\n";
        for ($i = $vardim; $i >0 ; $i--) {
          print "  " x $i ;
          print "}\n";
          print "  " x $i ;
          print q{printf(")");} . "\n";
        }
    } else {
      # Normal variable, might be character array. 
      print "  printf(\"" . $varname . " = ";
      if ($vartype eq "char") {
	  print "%s" ;
      } elsif ($vartype eq "int32") {
	  print "%d" ;
      } elsif ($vartype eq "float32") {
	  print "%g" ;
      } 
      # Last parameter
      if ($thisarg != @$fieldarray[$#$fieldarray]) {
        print ", ";
      } 
      print "\", $varname );\n";
    }
  }
  print 
q{  printf("\n");
  return ;
\}

};

  1;
};


sub emitMsgType ($;$)
{
  my $msgarray = $_[0];
  # optional string arg can replace the class name in the
  # message identifier string passed to VRPN
  my $msg_string_prefix = "";
  if (defined($_[1])) {
    $msg_string_prefix = $_[1];
  }
  # Array contains (line VRPN_MESSAGE (typeName InContactMode) { ... }
  # Extract the value of typeName and the arg list array (...)
  my $typeName = @$msgarray[2]->[1];
#  print STDOUT "$typeName, $msg_string_prefix\n";
  if ($headerFlag) {
    print "    vrpn_int32 d_" . $typeName . "_type;\n";
  } else {
    print "    d_" . $typeName . "_type = connection->register_message_type\n"
        . "       (\"" . ($msg_string_prefix ? "$msg_string_prefix" : 
                          ($className ? "$className ": "")) 
            . "$typeName\");\n"
  }
  1;
}

sub emitRegister ($)
{
  my $msgarray = $_[0];
  my $typeName = @$msgarray[2]->[1];
  if ($headerFlag) {
    print "    Register " . $typeName . "called in header ????\n";
  } else {
    print "    connection->register_handler(d_" . $typeName . "_type,\n"
        . "                    handle_" .$typeName . ",\n" 
        . "                    " . ($className eq "" ? "NULL" : "this") . ");\n"
  }
  1;
}

#--------------------------------------------------------------
# Emit the encode/decode/handlers at the right time from
# a pattern file. 
sub emitWriteWarning
{
  print "//Warning: this file automatically generated using" .
      " the command line\n" ;
  print "// ", $0, " ", join (' ',@ARGV), "\n";
  print "//DO NOT EDIT! Edit the source file instead.\n";
  1;
}

sub emitBaseClassHeader ($)
{
  local $className = $_[0];
  local $classPrefix = $className . "::";

  my $caps_class = $className;
  $caps_class =~ tr/[a-z]/[A-Z]/;
  print "#ifndef " . $caps_class . "_H\n";
  print "#define " . $caps_class . "_H\n";
  emitWriteWarning();
#  print "//Base class file generated with BASECLASS ". $_[0] .
#      " directive.\n\n";
  print 
q{
/*===3rdtech===
  Copyright (c) 2001 by 3rdTech, Inc.
  All Rights Reserved.

  This file may not be distributed without the permission of 
  3rdTech, Inc. 
  ===3rdtech===*/

#include <vrpn_Connection.h>

} ;

  emitAllDefines($className);

  print "\n\nclass $className \n{\n";
  print "  public:\n\n";
  print "    $className" . 
      " (vrpn_Connection * = NULL);\n" .
      "      // Constructor.\n";

  print "    virtual ~$className (void);\n" .
      "      // Destructor. HP compiler doesn't like it with '= 0;'\n\n";
  emitAllMsgType($className );
  print "\n\n";
  emitAllEnDecoders($className );
  print "};\n";
  print "#endif // ${caps_class}_H\n";
}

sub emitBaseClassCode ($)
{
  local $className = $_[0];
  local $classPrefix = $className . "::";

  my $caps_class = $className;
  $caps_class =~ tr/[a-z]/[A-Z]/;
  emitWriteWarning();
#  print "//Base class file generated with BASECLASS ". $_[0] .
#      " directive.\n\n";
  print 
q{
/*===3rdtech===
  Copyright (c) 2001 by 3rdTech, Inc.
  All Rights Reserved.

  This file may not be distributed without the permission of 
  3rdTech, Inc. 
  ===3rdtech===*/

#include <vrpn_Connection.h>
} ;
  print "#include \"${className}.h\"\n\n";
  print $classPrefix . $className . " (\n" .
      "   vrpn_Connection * connection)\n" .
      "{\n";

  print "  if (connection) {\n";

  emitAllMsgType($className );
  print "\n  }\n}\n\n";

  print $classPrefix . "~" . $className . " (void) { }\n\n";
  emitAllEnDecoders($className );
}

# Create header or C file for a base class.
# Base class registers message types, and includes encoders/decoders.
sub writeBaseClassFiles (;$)
{
  my $group_name;
  if (defined($_[0])) {
    $most_recent_tree = $_[0];
  }
  # Warning: we are opening and selecting a new file for output. 
  # Must go back to old file at end of function. 
  my $filehandle = select();
  open (BASEOUTF, ">" . $most_recent_tree . ($headerFlag ? ".h" : ".C")) or 
      die "Can't open base class output file";
  select(BASEOUTF);
  if ($headerFlag) {
    emitBaseClassHeader ($most_recent_tree);
  } else {
    emitBaseClassCode ($most_recent_tree);
  }
  close (BASEOUTF);

  # Go back to old file now.
  select($filehandle);
  print STDOUT "Output ". $most_recent_tree . ($headerFlag ? ".h" : ".C")
      . " base class file.\n";
  1;
}

sub emitAllDefines(;$) 
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  # This is different - we use a different key to get define list. 
  foreach ( @{$msg_trees{$most_recent_tree . "_defines"}}) {
    my $definearray = $_;
#    print STDOUT $definearray, "\n";
    my $defineName = @$definearray[2];
    my $defineVal = @$definearray[3];
    
    print "#define ${defineName} ${defineVal}\n";
  }
  1;
}

sub emitAllHandlers (;$)
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    #print $_ , "\n";
    emitHandler $_;
  }
  1;
}

sub emitAllMsgType (;$$)
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  # Second arg is optional string to use
  # when registering message types with the VRPN connection
  # Get msg string prefix from the grammar - it's "" if not included
  my $msg_string_prefix = $msg_trees{$most_recent_tree . "_msg_prefix"};
  if ($_[1]) {
    # Let command line over-ride
    $msg_string_prefix = $_[1];
  } 
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    if($msg_string_prefix) {
      emitMsgType($_, $msg_string_prefix);
    } else {
      emitMsgType($_);
    }
  }
  1;
}

sub emitAllRegister (;$)
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    emitRegister $_;
  }
  1;
}

sub emitAllReceivers (;$)
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    emitReceivers ($_);
  }
  1;
}

sub emitSingleReceiver ($;$$)
{
  my $msg_type = $_[0];
  my $namePostfix = "";
  if ($_[1]) {
    $namePostfix = $_[1];
  }
  if ($_[2]) {
    $most_recent_tree = $_[2];
  }
  my $found = 0;
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    #print STDOUT @$_[2]->[1] , "\n";
    if ($msg_type eq @$_[2]->[1]) {
      $found = 1;
      emitReceivers($_, $namePostfix, 0);
      last;
    }
  }
  if (!$found) {
    print STDOUT "Warning: Message $msg_type not found in message "
        . "group $most_recent_tree\n";
  }
  1;
}

sub emitAllEnDecoders (;$)
{
  if ($_[0]) {
    $most_recent_tree = $_[0];
  }
  foreach ( @{$msg_trees{$most_recent_tree}}) {
    emitEncoders $_;
    emitDecoders $_;
  }
  1;
}

# outputGrammar:
#   Parses C++ source files for directives to insert code fragments.
#   Recognized directives:
#     CLASSNAME <class name>
#       Subsequent functions definitions should be prefixed with <class name>::
#     NO CLASS
#       Cancels a CLASSNAME directive
#     ENCODE <type name>
#       Inserts a code fragment for a function
#         char * encode_<type name> (int * len, ...)
#         Allocates and returns a pointer to a character buffer holding
#         the marshalled message;  returns the length of the message in
#         len.  Components of the message are passed as the "..." arguments;
#         arrays should be passed as pointers.
#     DECODE <type name>
#       Inserts a code fragment for a function
#         int decode_<type name> (const char * buffer, ...)
#         Unpacks the buffer into its components, which should be passed
#         as pointers.  All arrays, whether fixed or variable-sized,
#         will be allocated on the heap and returned in pointers.
#     HANDLE <type name>
#     ...
#     }
#       Inserts a code fragment for a function
#         int handle_<type name> (void * userdata, vrpn_HANDLERPARAM p)
#         Declares the message's components on the stack and unpacks the
#         buffer into them.  Then falls through into the user code
#         indicated by "..." and terminated by "}".
#     REGISTER <type name>
#       Generates the following fragment:
#         vrpn_int32 <type name>_type;
#         <type name>_type = connection->register_message_type("<type name>");
#         connection->register_handler("<type name>", handle_<type name>,
#                                      NULL);
#       BUGS:  Does not allow specification of the userdata pointer (forces
#       it to NULL) or other parameters.


$outputGrammar = 
q{

  start: outfilename line(s)

  outfilename:
      /OUTPUT_FILENAME/ /(\S)+/
      {
        # Filename is not just word characters, but any non-space characters.
        $::outputFileName = $item[2];
        open (FH, ">" . $item[2]) || die "Can't open output file";
        select(FH);
        ::emitWriteWarning();
      }
  line:
    # Order is important - _decl must come first or it won't be matched. 
      msggroup
    | classname
    | defines
    | endecoders_decl
    | endecoders
    | handlers_decl
    | handlers
    | receivers_decl
    | receivers
    | receive_msg_header
    | receive_msg_body
    | receive_msg
    | register
    | msg_type_decl
    | msg_type
    | /[^\n]*\n/
      { # Print anything we don't recognize directly to the output file.  
        print " " x (1 + $prevcolumn - length($item[1]));
        print $item[1]; }

  msggroup:
      /USE_MSG_GROUP/ /(\S)+/
      { 
        ::parseMsgDefFile($item[2]);
      }

  classname:
      /CLASSNAME/ className
      { $::className = $item[2];
        $::classPrefix = $item[2] . "::"; }
    | /NO CLASS/
      { $::className = "";
        $::classPrefix = ""; }

#  prototype:
#      /DECLARE STRUCTURES/ messageName
#      { ::emitStructures ($item[2]); }

  defines:
      /DEFINES/ optGroupName
      { $::headerFlag = 0;
        ::emitAllDefines ($item[2]); }

  endecoders:
      /ENDECODERS/ optGroupName
      { $::headerFlag = 0;
        ::emitAllEnDecoders ($item[2]); }

  endecoders_decl:
      /ENDECODERS_DECL/ optGroupName
      { $::headerFlag = 1;
        ::emitAllEnDecoders ($item[2]); }

  handlers:
      /HANDLERS/ optGroupName
      { $::headerFlag = 0;
        ::emitAllHandlers($item[2]); }
    
  handlers_decl:
      /HANDLERS_DECL/ optGroupName
      { $::headerFlag = 1;
        ::emitAllHandlers($item[2]); }
    
  receivers:
      /RECEIVERS/ optGroupName
      { $::headerFlag = 0;
        ::emitAllReceivers($item[2]); }

  receive_msg_header:
      /RECEIVE_MSG_HEADER/ messageName optGroupName
      { $::headerFlag = 0;
        ::emitSingleReceiver($item[2], "_header", $item[3]); }

  receive_msg_body:
      /RECEIVE_MSG_BODY/ messageName optGroupName
      { $::headerFlag = 0;
        ::emitSingleReceiver($item[2], "_body", $item[3]); }

  receive_msg:
      /RECEIVE_MSG/ messageName optGroupName
      { $::headerFlag = 0;
        ::emitSingleReceiver($item[2], "", $item[3]); }

  receivers_decl:
      /RECEIVERS_DECL/ optGroupName
      { $::headerFlag = 1;
        ::emitAllReceivers($item[2]); }

  register:
      /REGISTER/ optGroupName 
      { $::headerFlag = 0;
        ::emitAllRegister ($item[2]); }

  msg_type:
      /MSG_TYPE/ /".*"/ optGroupName 
      { $::headerFlag = 0;
        $item[2] =~ tr/\"//d;
        ::emitAllMsgType ($item[3], $item[2] ); }
    | /MSG_TYPE/ optGroupName 
      { $::headerFlag = 0;
        ::emitAllMsgType ($item[2]); }

  msg_type_decl:
      /MSG_TYPE_DECL/ optGroupName 
      { $::headerFlag = 1;
        ::emitAllMsgType ($item[2]); }

  groupName:
      /(\w)+/
      { $return = $item[1]; }

  optGroupName:
      # Group name is optional - just uses previous if blank.
      /(\w)+/ /[;\{]/
      { $return = $item[1]; }
    | /[;\{]/
      { $return = ""; }

  className:
      /(\w)+/
      { $return = $item[1]; }

  messageName:
      /(\w)+/
      { $return = $item[1]; }

  optionalVariableName:
      /(\w)+/ /;/
      { $return = $item[1]; }
    |  /;/
      { $return = "NULL"; }
};

$outputParser = new Parse::RecDescent ($outputGrammar);

#$::RD_HINT = 1;     # turn on verbose debugging
#$::RD_TRACE = 1;     # turn on verbose debugging

#--------------------------------------------------------------
# Do the actual work.
$outputInput = "";
$outputFileName = "";
$base_c_flag = 0;
$base_h_flag = 0;
$msg_def_file = "";
$spec_file = "";
process_args();

if ($base_h_flag) {
  # Create a base-class header. 
  $headerFlag = 1;
  parseMsgDefFile($msg_def_file);
  writeBaseClassFiles();
}
if ($base_c_flag) {
  # Create a base-class C. 
  $headerFlag = 0;
  parseMsgDefFile($msg_def_file);
  writeBaseClassFiles();
}

if ($spec_file) {
  $headerFlag = 0;
  open(FH, "< " . $spec_file);
  while (<FH>) {
    $outputInput .= $_;
  };
  close(FH);

  # Parse the file, and output modified file using grammar
  # $outputParser->start returns undef if it can't parse the file. 
  if(defined( $outputParser->start($outputInput))) {
    # Close the file that was opened by the grammar.
    close(FH);
    print STDOUT "Parsed " .$spec_file . " and output ". $outputFileName . "\n";
    exit 0;
  } else {
    die "Unable to parse the input file " . $spec_file . "\n";
  }
}

#----------------------------------------------------------------------------
sub process_args {
    $verbose = 1; #$debugging = 0;
    my @myargv = @ARGV;
    while (@myargv) {
        $a = shift @myargv;
        if (($a eq "-c")||($a eq "-C")) {
            $base_c_flag = 1;
            $msg_def_file = shift @myargv ;
        } elsif ($a eq "-h") {
            $base_h_flag = 1;
            $msg_def_file = shift @myargv ;
        } elsif ($a eq "-q") {
            $verbose = 0;
        } else {
            $spec_file = $a;
        }
    }
    if (!$spec_file && !$base_c_flag && !$base_h_flag) {
      print "No files specified, no output produced\n";
      Usage();
    }
    if ($base_c_flag && !(-e $msg_def_file)) {
      print "Non-existent message definition file specified with -c option\n.";
      Usage();
    }
    if ($base_h_flag && !(-e $msg_def_file)) {
      print "Non-existent message definition file specified with -h option\n.";
      Usage();
    }
    if ($spec_file && !(-e $spec_file)) {
      print "Non-existent input file specified \n.";
      Usage();
    }
}

#----------------------------------------------------------------------------
sub Usage {
    print STDERR <<"_EOT_";

Usage: gen_vrpn_rpc <options>
 where options are: 
 -c <msg def file>       : Output a base class C file from a msg def file
 -h <msg def file>       : Output a base class header file from a msg def file
 <code file>             : file with vrpn_rpc directives to be translated

Generally, specify only one of the options to output one file. 
_EOT_
    exit(1);
}
