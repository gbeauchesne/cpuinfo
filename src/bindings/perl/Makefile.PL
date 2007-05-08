use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

my $cflags = 'Os';
my $cpuinfo_incdir = '/usr/include';
my $cpuinfo_libdir = '';
my $cpuinfo_consts = 'constants.h';

while ($ARGV[0] =~ /^--/) {
    $_ = shift;
    if (m|^--cflags=(.+)|) {
	$cflags = $1;
    }
    elsif (m|^--cpuinfo-incdir=(.+)|) {
	$cpuinfo_incdir = $1;
    }
    elsif (m|^--cpuinfo-libdir=(.+)|) {
	$cpuinfo_libdir = $1;
    }
}

my $CPUINFO_CFLAGS		= "-Wall -I${cpuinfo_incdir}";
my $CPUINFO_LIBS		= ($cpuinfo_libdir ? "-L${cpuinfo_libdir} " : "") . "-lcpuinfo";

WriteMakefile('NAME'		=> "Cpuinfo",
	      'OPTIMIZE'	=> "$cflags",
	      'INC'		=> "$CPUINFO_CFLAGS",
	      'LIBS'		=> ["$CPUINFO_LIBS"],
	      'DEFINE'		=> '',
	      'clean'		=> { FILES => "\$(cpuinfo_consts)" },
	      'depend'		=> { 'Cpuinfo.o' => "\$(cpuinfo_consts)" },
	      'macro'		=> { 'cpuinfo_incdir' => $cpuinfo_incdir,
				     'cpuinfo_libdir' => $cpuinfo_libdir,
				     'cpuinfo_consts' => $cpuinfo_consts },
	      );

package MY;

sub postamble {
    return << 'EOF'
$(cpuinfo_consts): $(cpuinfo_incdir)/cpuinfo.h constants.gen
	$(PERL) constants.gen $< $@
EOF
}
