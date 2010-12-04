#!/usr/local/bin/perl

# Perl script to parse VRPN message parameter declarations
# from a definition file (def-file) and then build
# an associative array to define them.
# The script then reads in one or more code files
# and inserts code in them based on keywords in the code
# files and the message parameters defined in the def-file.

# usage:  def.pl <def-file-name> <code-file-name>+

# Tom Hudson, June 1999

# Recursive descent parser pointed out by Chris Weigle.
# A redesign might use half-a-dozen RDParsers instead of two.

use Parse::RecDescent;
#$::RD_HINT = 1;     # turn on verbose debugging
#$::RD_TRACE = 1;     # turn on verbose debugging

local (%messageDefinitionArray) = ();
local ($numDefinitions) = 0;

# debugging flags

local ($dumpMessageArray) = 0;

# Tie::IxHash allows us to retrieve from the hash table in
# insertion order, which would be esp. nice for debugging.

# use Tie::IxHash;
# tie %messageDefinitionArray, "Tie::IxHash";

# If this were a full-featured language, we'd use a data structure
# to hold the syntax tree we create for the data types.  However,
# Perl (to the best of my knowledge) doesn't have data structures,
# so we have to encode it in a string, then parse that string
# every time we want to read it.
# One way to do this would be to have a recursive descent parser
# for every output, but instead we rewrite the standard definition
# into a simpler format that we don't need a RD parser to parse.
# BNF looks something like:
#  typeRecord:       typeEntry
#                  | typeEntry ':' typeRecord
#  typeEntry:        typeName variableName optionalArray
#                  | 'struct' typeName '{'
#                     structTypeRecord
#                    '}' variableName optionalArray
#  structTypeRecord: typeEntry
#                  | typeEntry ';' structTypeRecord
#  optionalArray:    count
#                  | empty
# Probably should have done it using many RD parsers, especially
# since they seem to be reentrant.

$definitionGrammar = q[

  start: line(s)

  line: /VRPN_MESSAGE/ typeName /{/ structOrFieldSequence /}/
    { $main::messageDefinitionArray{$item[2]} = $item[4];
      #  print $item[2] . " is " . $item[4] . "\n";
      $main::numDefinitions++; }
    | /[^\n]*\n/
    { }

  typeName: /(\w)+/
    { $return = $item[1]; }

  structOrFieldSequence:
      structDefinition structOrFieldSequence
      { $return = $item[1] . $item[2]; }
    | field structOrFieldSequence
      { $return = $item[1] . $item[2]; }
    | structDefinition
      { $return = $item[1]; }
    | field
      { $return = $item[1]; }
    | //
      { $return = ""; }

  structDefinition:
      /struct/ structTypeName /{/
        fieldSequence
      /}/ structName count
      { $return = "struct " . $item[2] . " {"
                  . join(";", split(/:/, $item[4]))
                  . "}" . $item[6] . " " . $item[7]; }

  structName:
      /(\w)+/
      { $return = $item[1]; }

  structTypeName:
      /(\w)+/
      { $return = $item[1]; }

  fieldSequence:
      field fieldSequence
      { $return = $item[1] . $item[2]; }
    | field
      { $return = $item[1]; }

  field:
      /(\w)+/ /(\w)+/ optionalCount
      { $return = $item[1] . " " . $item[2] . " " . $item[3] . ":" . $item[4]; }

  optionalCount:
      count optionalCount
      { $return = $item[1] . $item[2]; }
    | //
      { $return = ""; }

  count:
      /\[/ /(\d)+/ /\]/ # fixed, numeric count
      { $return = "[" . $item[2] . "]"; }
    | /\[/ /(\w)+/ /\]/ # variable count
      { $return = "[" . $item[2] . "]"; }

];

$definitionParser = new Parse::RecDescent ($definitionGrammar);
$definitionInput = "";


#   take name of file to open for definitions from command line $ARGV[0]

open(DEFHANDLE, "< " . $ARGV[0]);
while (<DEFHANDLE>) {
  $definitionInput .= $_;
};
close(DEFHANDLE);

$definitionParser->start($definitionInput);

# traverse messageDefinitionArray and print

if ($dumpMessageArray) {
  foreach my $typeName (keys(%messageDefinitionArray)) {
    print $typeName . " is:\n";
    # print $typeName . " is " . $messageDefinitionArray{$typeName} . "\n";
    foreach my $arg (split(/:/, $messageDefinitionArray{$typeName})) {
      if (substr($arg,0,6) eq "struct") {
        my $openbrace = index($arg, "{");
        my $closebrace = index($arg, "}");  # must only be one!
        my $openbracket = index($arg, "[", $closebrace);
        my $argbody = substr($arg, $openbrace + 1,
                             $closebrace - $openbrace - 1);
        my $structtypename = substr($arg, 7, $openbrace - 7);
        my $structname = substr($arg, $closebrace + 1,
                         $openbracket - $closebrace - 2);
        my $count = substr($arg, $openbracket);
        # print $argbody;
        print "  struct " . $structtypename . " {\n";
        foreach my $subarg (split(/;/, $argbody)) {
          print "    " . $subarg . "\n";
        }
        print "  } " . $structname
              . " " . $count . "\n";
      } else {
        print "  " . $arg . "\n";
      }
    }
  }
}

# print "Parsed " . $numDefinitions . " definitions.\n";

# Emits a function to decode the named type into network form.
# Returns 0, or -1 on error.
#
#  int myClass::decode_foo (const char * buffer, ...) {
#    if (!buffer || ...) return -1;
#    vrpn_unbuffer(&buffer, ...);
#    return 0;
#  }

sub emitDecoder {
  my $typeName = $_[0];

  # function declaration / parameter list

  print "int " . $classPrefix . "decode_" . $typeName . " (\n";
  print "    const char * buffer,\n";

  my @argarray = split(/:/, $messageDefinitionArray{$typeName});

  for ($i = 0; $i <= $#argarray; $i++) {
    my @thisarg = split(/ /, $argarray[$i]);
    print "    ";

    my $varname = $thisarg[1];

    if (substr($thisarg[0],0,6) eq "struct") {
      print "struct " . $varname . " ";
      my $closebrace = index($argarray[$i], "}");
      $openbracket = index($argarray[$i], "[", $closebrace);
      $varname = substr($argarray[$i], $closebrace + 1,
                        $openbracket - $closebrace - 2);

    } else {
      $openbracket = index($argarray[$i], "[");
      if ($thisarg[0] ne "char") {
        print "vrpn_"
      }
      print $thisarg[0] . " ";
    }

    print "* " . $varname;
    if ($i != $#argarray) {
      print ",";
    }
    print "\n";
  }

  print ") {\n";

  # function body

  # test for non-NULL

  print "  if (!buffer";
  for ($i = 0; $i <= $#argarray; $i++) {
    my @thisarg = split(/ /, $argarray[$i]);

    my $varname = $thisarg[1];

    if (substr($thisarg[0],0,6) eq "struct") {
      $closebrace = index($argarray[$i], "}");
      $openbracket = index($argarray[$i], "[", $closebrace);
      $varname = substr($argarray[$i], $closebrace + 1,
                        $openbracket - $closebrace - 2);
    }
    print "\n   || !" . $varname;
  }
  print ") return -1;\n";

  # OK, tough stuff:  unbuffer each entry.
  # Need to handle nested loops!

  emitUnbufferList(@argarray);

  print "  return 0;\n";
  print "}\n";
  print "\n";

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

sub emitEncoder {
  my $typeName = $_[0];

  # function declaration / parameter list

  print "char * " . $classPrefix . "encode_" . $typeName . " (\n";
  print "    int * len,\n";

  my @argarray = split(/:/, $messageDefinitionArray{$typeName});

  for ($i = 0; $i <= $#argarray; $i++) {
    my @thisarg = split(/ /, $argarray[$i]);
    print "    ";

    my $varname = $thisarg[1];
    my $openbracket = $[-1;

    if (substr($thisarg[0],0,6) eq "struct") {
      print "struct " . $varname . " ";
      my $closebrace = index($argarray[$i], "}");
      $openbracket = index($argarray[$i], "[", $closebrace);
      $varname = substr($argarray[$i], $closebrace + 1,
                        $openbracket - $closebrace - 2);

    } else {
      $openbracket = index($argarray[$i], "[");
      if ($thisarg[0] ne "char") {
        print "vrpn_"
      }
      print $thisarg[0] . " ";
    }
    if ($openbracket != $[-1) {
      print "* ";
    }
    print $varname;
    if ($i != $#argarray) {
      print ",";
    }
    print "\n";
  }

  print ") {\n";

  # function body

  print "  char * msgbuf = NULL;\n";
  print "  char * mptr;\n";
  print "  int mlen;\n";
  print "  if (!len) return NULL;\n";

  # figure out size of message by adding up types of every entry.

  print "  *len =\n";
  emitMsgSizes(@argarray);

  print "  msgbuf = new char [*len];\n"
      . "  if (!msgbuf) {\n"
      . "    fprintf(stderr, \"encode_" . $typeName . ":  " .
                     "Out of memory.\\n\");\n"
      . "    *len = 0;\n"
      . "  } else {\n"
      . "    mptr = msgbuf;\n"
      . "    mlen = *len;\n";

  # OK, tough stuff:  buffer each entry.
  # Need to handle nested loops!

  emitBufferList(@argarray);

  print "  }\n";
  print "  return msgbuf;\n";
  print "}\n";
  print "\n";

  1;
};

# Emits a function that unmarshalls (see decode_foo) onto the stack
# then executes user-specified code.

sub emitHandler {
  my $typeName = $_[0];

  # function declaration / parameter list

  print "\n";
  print "int " . $classPrefix . "handle_" . $typeName . " ("
               . "void * userdata, vrpn_HANDLERPARAM p) {\n";
  print "  const char * buffer = p.buffer;\n";

  my @argarray = split(/:/, $messageDefinitionArray{$typeName});

  for (my $i = 0; $i <= $#argarray; $i++) {
    my @thisarg = split(/ /, $argarray[$i]);

    my $varname = $thisarg[1];

    my @arrayDimensions = split(/\[/, $argarray[$i]);
    my $numArrayDimensions = $#arrayDimensions;

    #print "ARGARRAY[I] = " . $argarray[$i] . "\n";
    #foreach my $ad (@arrayDimensions) {
      #print "DIMENSION = " . $ad . "\n";
    #}
    #print "# ARRAY DIMENSIONS = " . $numArrayDimensions . "\n";

    print "  ";
    if (substr($thisarg[0],0,6) eq "struct") {
      print "struct " . $varname . " ";
      my $closebrace = index($argarray[$i], "}");
      $openbracket = index($argarray[$i], "[", $closebrace);
      $varname = substr($argarray[$i], $closebrace + 1,
                        $openbracket - $closebrace - 2);

      $trailingBrace = rindex($argarray[$i], "}");
      $trailingPart = substr($argarray[$i], $trailingBrace);
      @arrayDimensions = split(/\[/, $trailingPart);
      $numArrayDimensions = $#arrayDimensions;
    } else {
      if ($thisarg[0] ne "char") {
        print "vrpn_"
      }
      print $thisarg[0] . " ";
    }

    # hack:  assume array dimension is a constant if
    # we can get a nonzero numeric value for each of the dimensions

    my $dimensionIsConstant = 1;
    for (my $j = 1; $j <= $numArrayDimensions; $j++) {
      my $ad = $arrayDimensions[$j];
      #print "DIMENSION = " . $ad . "\n";
      if ($ad == 0) {
        $dimensionIsConstant = 0;
      }
    }
      
    if (($numArrayDimensions > 0) &&
        !$dimensionIsConstant) {
      for (my $j = 1; $j <= $numArrayDimensions; $j++) {
        print "*";
      }
      print " ";
    }

    print $varname;

    if (($numArrayDimensions > 0) &&
        $dimensionIsConstant) {
      print " ";
      print substr($argarray[$i], index($argarray[$i], "["));
    }

    print ";\n";
  }

  print "\n";

  # function body

  # test for non-NULL

  # OK, tough stuff:  unbuffer each entry.
  # Need to handle nested loops!

  emitAndedUnbufferList(@argarray);

  # User is expected to write the rest of the code.

  print "\n";

  1;
};

#
# TODO:  extra parameters for register_handler
#

sub emitRegister {
  my $typeName = $_[0];
  my $argument = $_[1];

  my $qTypeName = $typeName . "_type";
  print "  vrpn_int32 " . $qTypeName . ";\n";
  print "  " . $qTypeName . " = connection->register_message_type(\""
             . $typeName . "\");\n";
  print "  connection->register_handler(" . $qTypeName . ", handle_"
             . $typeName . ", " . $argument . ");\n";

}

# Given a message type name, write out declarations for all the
# structures the message uses.

# Quick 'n' dirty;  doesn't pretty-print.

# Need to do three ugly things:
#   Find variable-sized arrays in the declaration and replace them
# with pointers.
#   Insert "vrpn_" in type names where appropriate
# x Insert trailing ';' before the closing '}'

sub emitStructures {
  my $typeName = $_[0];

  my @argarray = split(/:/, $messageDefinitionArray{$typeName});

  foreach my $arg (@argarray) {
    my @thisarg = split(/ /, $arg);
    if ($thisarg[0] eq "struct") {
      my $openBrace = index($arg, "{");
      my $closeBrace = rindex($arg, "}");
      #print substr($arg, 0, $closeBrace) . ";};\n";

      my $structFieldString = substr($arg, $openBrace + 1,
                                     $closeBrace - $openBrace - 1);
      my $structDeclaration = substr($arg, 0, $openBrace);

      #print "FIELDS = " . $structFieldString . "\n";
      #print "DECLARATION = " . $structDeclaration . "\n";

      my @structFields = split(/;/, $structFieldString);

      print $structDeclaration . " {\n";

      foreach my $field (@structFields) {
        my @words = split(/ /, $field);
        my $typename = $words[0];
        my $varname = $words[1];
        my $arrayspec = $words[2]; 
        if ($typename eq "struct") {
          $typename = $words[0] . " " . $words[1];
          $varname = $words[2];
          $arrayspec = $words[3];
        } elsif ($typename ne "char") {
          $typename = "vrpn_" . $typename;
        }

        # figure out if nonconstant-sized array

        # hack:  assume array dimension is a constant if
        # we can get a nonzero numeric value for each of the dimensions

        my @counts = split(/]/, $arrayspec);
        my $numopenbracket = $#counts;

        my $dimensionIsConstant = 1;
        for (my $j = 0; $j <= $numopenbracket; $j++) {
          my $ad = substr($counts[$j], 1);
          #print "DIMENSION = " . $ad . "\n";
          if ($ad == 0) {
            $dimensionIsConstant = 0;
          }
        }

        if (!$dimensionIsConstant) {
          $varname = " " . $varname;
          for (my $j = 0; $j <= numopenbracket; $j++) {
            $varname = "*" . $varname;
          }
          $arrayspec = "";
        }
          

        print "  " . $typename . " " . $varname . " " . $arrayspec . ";\n";

      }

      print "};\n";
      print "\n";
    }
  }
}


# Figure out how large the message will be once we encode it:
#   take sizeof every element and sum.

sub emitMsgSizes {
  my @argarray = @_;
  # print "Sizes:  " . $#argarray . "\n";

  for (my $i = 0; $i <= $#argarray; $i++) {
    my @thisarg = split(/ /, $argarray[$i]);
    print "    sizeof(";

    my $typename = $thisarg[0];
    my $openbracket = $[-1;

    if ($typename eq "struct") {
      print "struct " . $thisarg[1];
      my $closebrace = index($argarray[$i], "}");
      $openbracket = index($argarray[$i], "[", $closebrace);
    } else {
      $openbracket = index($argarray[$i], "[");
      if ($typename ne "char") {
        print "vrpn_"
      }
      print $typename;
    }
    print ")";

    # multiply out all the counts
    # This can be numeric or variable, so we can't do the math here,
    # but have to do it at runtime.

    my $count = 1;
    my $countstr = $thisarg[$#thisarg];
    if ($openbracket != $[-1) {
      foreach my $countsubstr (split (/]/, $countstr)) {
        print " * " . substr($countstr, 1, length($countstr) - 2);
      }
    }

    if ($i != $#argarray) {
      print " +";
    } else {
      print ";";
    }
    print "\n";
  }
};

# Emit code to buffer (marshal) "one thing"
# Arguments:
#    thing prefix indent
#      thing is a complex space-separated string (don't you love Perl?):
#        <type name> <variable name> [<array specifications>]
#      prefix is a structure name (possibly with array index)
#      indent is a string of spaces that we use to make pretty output

sub emitBufferItem {
  my $arg = $_[0];
  my $prefix = $_[1];
  my $indent = $_[2];

  my @thisarg = split(/ /, $arg);

  my $typename = $thisarg[0];
  my $varname = $thisarg[1];
  my $openbracket = index($arg, "[");

  if ($openbracket != $[-1) {
    # is it a one-dimensional character array?
    my $lastopenbracket = rindex($arg, "[");
    if (($typename eq "char") &&
        ($lastopenbracket == $openbracket)) {
      my $count = substr($arg, $openbracket + 1,
                         index($arg, "]") - $openbracket - 1);
      print $indent . "    vrpn_buffer(&mptr, &mlen, " . $prefix . $varname
                           . ", " . $count . ");\n";
    } elsif ($typename eq "struct") {
      # array of structures;  assume one-dimensional

      print $indent . "    int s;\n";
      print $indent . "    for (s = 0; s < 1";
      my $countstr = $thisarg[$#thisarg];
      foreach my $countsubstr (split (/]/, $countstr)) {
         print " * " . substr($countstr, 1, length($countstr) - 2);
      }
      print "; s++) {\n";

      my $openbrace = index($arg, "{");
      my $lastclosebrace = rindex($arg, "}");
      my $subargstr = substr($arg,
                             $openbrace + 1,
                             $lastclosebrace - $openbrace - 2);
      # print "Subargstr:  " . $subargstr . "\n";
      my @subarg = split(/;/, $subargstr);
      # print "#subarg:  " . $subarg . "\n";

      $openbracket = index($arg, "[", $lastclosebrace);
      my $structname = substr($arg, $lastclosebrace + 1,
                       $openbracket - $lastclosebrace - 2);


      for (my $i = 0; $i <= $#subarg; $i++) {
        # print $i . " emitting: " . $subarg[$i] . "\n";
        emitBufferItem($subarg[$i], $structname . "[s].");
      }

      print "    }\n";
    } else {

      # either an array of non-char
      # or a multidimensional array of any type
      # or ...

      print "BUG BUG BUG\n";

    }
  } else {

    # default case:  not a struct, not an array, not anything special,
    # just an ordinary parameter

    print $indent . "    vrpn_buffer(&mptr, &mlen, "
                  . $prefix . $varname . ");\n";
  }

};

# Given a list of things to buffer,
# generate the code to buffer each.

sub emitBufferList {
  my @argarray = @_;

  for (my $i = 0; $i <= $#argarray; $i++) {
     emitBufferItem($argarray[$i], "", "");
  }
};

# Emit code to unbuffer (demarshal) "one thing"
#

sub emitUnbufferItem {
  my $arg = $_[0];
  my $prefix = $_[1];
  my $indent = $_[2];
  my $arrayspec = $_[3];
  my $recursioncount = $_[4];
  my $dereferenceNonconstantArraySizes = $_[5];

  my @thisarg = split(/ /, $arg);

  my $typename = $thisarg[0];
  my $varname = $thisarg[1];
  my $firstOpenBracket = index($arg, "[");
  my $lastclosebrace = rindex($arg, "}");
  my $trailingOpenBracket = index($arg, "[", $lastclosebrace);
  my $lastopenbracket = rindex($arg, "[");

  my $qualifiedVarname = $prefix . $varname . $arrayspec;

  # is it not an array?
  if ($trailingOpenBracket == $[-1) {

    if ($typename eq "struct") {

      # struct

      my $openbrace = index($arg, "{");
      my $subargstr = substr($arg,
                             $openbrace + 1,
                             $lastclosebrace - $openbrace - 1);
      my @subarg = split(/;/, $subargstr);

      #print "ARG = " . $arg . "\n";

      my $structname = substr($arg, $lastclosebrace + 1);
                       # $trailingOpenBracket - $lastclosebrace - 2);

      #print "STRUCTNAME = " . $structname . "\n";

      for (my $i = 0; $i <= $#subarg; $i++) {
        emitUnbufferItem($subarg[$i], $prefix . $structname . $arrayspec . ".",
                         "  " . $indent, "", $recursioncount + 1);
        # this doesn't need to be $recursioncount + 1, it could just be
        # $recursioncount, since we're really using it for a
        # number-of-enclosing-loops count, but it doesn't hurt.
      }

    } else {

      # default case:  not a struct, not an array, not anything special

      print $indent . "  vrpn_unbuffer(&buffer, " . $qualifiedVarname . ");\n";

    }

  } elsif (($typename eq "char") &&
          ($lastopenbracket == $trailingOpenBracket)) {

    # one-dimensional character array

    my $count = substr($arg, $trailingOpenBracket + 1,
                       index($arg, "]") - $trailingOpenBracket - 1);

    # hack:  $count == 0 means this array size is nonconstant
    if ($dereferenceNonconstantArraySizes && ($count == 0)) {
      $count = "*" . $count;
    }

    # hack:  remove any leading '&'
    # (But hey, strings are a little non-orthogonal.  We could probably
    #  just go ahead and unbuffer one character at a time inside a loop,
    #  but it's more efficient to take advantage of vrpn_unbuffer()'s
    #  char special case.  If this ever breaks, remove the char special
    #  case and add a single-character unbuffer function to vrpn.)

    my $charVarname = $qualifiedVarname;
    if (index($qualifiedVarname, "&") == 0) {
      $charVarname = substr($charVarname, 1);
    }

    # if the dimension is non-constant we have to allocate the array.

    if ($count == 0) {
      print $indent . "  " . $charVarname . " = new char [" . $count . "];\n";
    }

    print $indent . "  vrpn_unbuffer(&buffer, " . $charVarname .
                    ", " . $count . ");\n";

  } else {

    # array:
    #   emit code to allocate the array on the heap IFF size is non-constant
    #   emit code for an iterator (variable declaration and for loop)
    #   construct an $arrayspec for this array dimension
    #   change the string we pass on to not include this dimension
    #   recurse

    #print "ARG = " . $arg . "\n";

    my $countstr = $thisarg[$#thisarg];
    my @counts = split(/]/, $countstr);
    my $numopenbracket = $#counts;

    my $numdimensions = $numopenbracket;

    my $loopvar = "vd_i" . $recursioncount;

    # allocation time

    # hack:  assume array dimension is a constant if
    # we can get a nonzero numeric value for each of the dimensions

    my $dimensionIsConstant = 1;
    for (my $j = 0; $j <= $numopenbracket; $j++) {
      my $ad = $counts[$j];
      #print "DIMENSION = " . $ad . "\n";
      if ($ad == 0) {
        $dimensionIsConstant = 0;
      }
    }

    my $thisCount = substr($counts[0], 1);
    
    if (!$dimensionIsConstant) {
      if ($dereferenceNonconstantArraySizes) {
        $thisCount = "*" . $thisCount;
      }

      my $bareVarname = $qualifiedVarname;
      if (index($bareVarname, "&") == 0) {
        $bareVarname = substr($bareVarname, 1);
      }

      my $effectiveTypeName = $typename;
      #print "BVN = " . $bareVarname . "\n";
      #print "ETN = " . $effectiveTypeName . "\n";

      if ($effectiveTypeName eq "struct") {

        # effectiveTypeName and bareVarname reverse

        $effectiveTypeName = $bareVarname;

        $bareVarname = substr($arg, $lastclosebrace + 1);
        #print "BVN = " . $bareVarname . "\n";
        #print "ETN = " . $effectiveTypeName . "\n";

        my $bracketIndex = index($bareVarname, "[");
        if ($bracketIndex != $[-1) {
          $bareVarname = substr($bareVarname, 0, $bracketIndex);
          #print "ETN = " . $effectiveTypeName . "\n";
        }
      } elsif ($effectiveTypeName ne "char") {
        $effectiveTypeName = "vrpn_" . $effectiveTypeName;
      }

      print $indent . "  " . $bareVarname . " = new "
                    . $effectiveTypeName . " ";

      # print any necessary "*"s
      for (my $i = 1; $i <= $#counts; $i++) {
        print "*";
      }

      print " [" . $thisCount . "];\n";
    }

    # define an index variable for this dimension of the array
    print $indent . "  int " . $loopvar . ";\n";

    # start a loop over this dimension of the array
    print $indent . "  for (" .
          $loopvar . " = 0;  " .
          $loopvar . " < " . $thisCount . ";  " .
          $loopvar . "++) {\n";

    # "fix up" string we're passing down and construct the new arrayspec
    # The string we pass down to the next recursion should have the FIRST
    # array specifier ([...]) removed

    my $trailingCloseBracket = index($arg, "]", $trailingOpenBracket);

    #print "TRAILING OPEN = " . $trailingOpenBracket . "\n";
    #print "TRAILING CLOSED = " . $trailingCloseBracket . "\n";

    my $newarg = substr($arg, 0, $trailingOpenBracket) .
                 substr($arg, $trailingCloseBracket + 1);
    my $newarrayspec = $arrayspec . "[" . $loopvar . "]";

    # recurse
    emitUnbufferItem($newarg, $prefix,
                     $indent . "  ",
                     $newarrayspec,
                     $recursioncount + 1,
                     $dereferenceNonconstantArraySizes);

    # end the for loop
    print $indent . "  }\n";

  }

};

# Given a list of things emit unbuffering code for all of them.

sub emitUnbufferList {
  my @argarray = @_;

  for (my $i = 0; $i <= $#argarray; $i++) {
     emitUnbufferItem($argarray[$i], "", "", "", 0, 1);
  }

};

# Given a list of things emit unbuffering code for all of them
# with & prefixed.  (A hack inside emitUnbufferItem will remove the
# '&' before one-dimensional character arrays.)

sub emitAndedUnbufferList {
  my @argarray = @_;

  for (my $i = 0; $i <= $#argarray; $i++) {
     emitUnbufferItem($argarray[$i], "&", "", "", 0, 0);
  }

};

local $class = "";
local $classPrefix = "";

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


$outputGrammar = q[

  start: line(s)

  line:
      classname
    | prototype
    | encoder
    | decoder
    | handler
    | register
    | /[^\n]*\n/
      { my $thisspacepad = "";
        for (my $i = 0 ; $i < 1 + $prevcolumn - length($item[1]) ; $i++) {
          $thisspacepad .= " ";
        }
        print $thisspacepad . $item[1]; }

  classname:
      /CLASSNAME/ className
      { my $class = className;
        my $classPrefix = className . "::"; }
    | /NO CLASS/
      { my $class = "";
        my $classPrefix = ""; }

  prototype:
      /DECLARE STRUCTURES/ messageName
      { main::emitStructures ($item[2]); }

  encoder:
      /ENCODE/ typeName
      { main::emitEncoder ($item[2]); }

  decoder:
      /DECODE/ typeName
      { main::emitDecoder ($item[2]); }

  handler:
      /HANDLE/ typeName
      { main::emitHandler ($item[2]); }

  register:
      /REGISTER/ typeName optionalVariableName
      { main::emitRegister ($item[2], $item[3]); }

  typeName:
      /(\w)+/
      { $return = $item[1]; }

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
];

$outputParser = new Parse::RecDescent ($outputGrammar);
$outputInput = "";


#$::RD_HINT = 1;     # turn on verbose debugging
#$::RD_TRACE = 1;     # turn on verbose debugging

# Do the actual work.
#print "Entering loop.\n";

for (my $i = 1; $i <= 1; $i++) {

  # print "Reading " . $ARGV[$i] . ".\n";

  open(OUTPUTHANDLE, "< " . $ARGV[$i]);
  while (<OUTPUTHANDLE>) {
    $outputInput .= $_;
  };
  close(OUTPUTHANDLE);

  $outputParser->start($outputInput);

}

# tests
#emitEncoder("InDirectZControl");
#emitEncoder("HelloMessage");
#emitEncoder("ReportScanDatasets");
#emitDecoder("InDirectZControl");
#emitDecoder("HelloMessage");
#emitDecoder("ReportScanDatasets");

1;



