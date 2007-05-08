package Cpuinfo;

use strict;
use vars qw(@ISA);
use DynaLoader;

@ISA = qw(DynaLoader);
Cpuinfo->bootstrap();
1;
