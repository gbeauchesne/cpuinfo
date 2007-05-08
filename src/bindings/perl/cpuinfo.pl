#!/usr/bin/perl
#*****************************************************************************
#
#  cpuinfo - Processor identification
#
#  cpuinfo (C) 2006-2007 Gwenole Beauchesne
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2, as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#*****************************************************************************

use strict;
use Cpuinfo;
use Data::Dumper;

my $cpuinfo = Cpuinfo::new();

print "Processor Information\n";

my $vendor = $cpuinfo->get_vendor();
print "  Model: ", Cpuinfo::string_of_vendor($vendor), " ", $cpuinfo->get_model();
my $freq = $cpuinfo->get_frequency();
if ($freq > 0) {
    print ", ";
    if ($freq > 1000) {
        print $freq / 1000, " GHz";
    } else {
        print $freq, " MHz";
    }
}
print "\n";

my $socket = $cpuinfo->get_socket();
print "  Package: ";
print Cpuinfo::string_of_socket($socket), ", " if $socket != Cpuinfo::SOCKET_UNKNOWN;
my $n_cores = $cpuinfo->get_cores();
print $n_cores, " Core";
print "s" if $n_cores > 1;
my $n_threads = $cpuinfo->get_threads();
print ",", $n_threads, " Threads per Core" if $n_threads > 1;
print "\n";

print "\n";
print "Processor Caches\n";

foreach my $cache ($cpuinfo->get_caches()) {
    if ($cache->{level} == 0 && $cache->{type} == Cpuinfo::CACHE_TYPE_TRACE) {
        print "  Instruction trace cache, ", $cache->{size}, " uOps";
    } else {
        print "  L", $cache->{level}, " ", Cpuinfo::string_of_cache_type($cache->{type}), " cache, ";
        my $size = $cache->{size};
        if ($size >= 1024) {
            print $size / 1024, " MB";
        }
        else {
            print $size, " KB";
        }
    }
    print "\n";
}

print "\n";
print "Processor Features\n";

sub print_features($$$) {
    my ($cpuinfo, $min, $max) = @_;
    for my $feature ($min .. $max) {
        if ($cpuinfo->has_feature($feature)) {
            my $name = Cpuinfo::string_of_feature($feature);
            my $detail = Cpuinfo::string_of_feature_detail($feature);
            if ($name && $detail) {
                printf "  %-10s %s\n", $name, $detail;
            } else {
                printf "  %-10s No description for feature %08x\n", "<error>", $feature;
            }
        }
    }
}

print_features $cpuinfo, Cpuinfo::FEATURE_COMMON + 1, Cpuinfo::FEATURE_COMMON_MAX;
print_features $cpuinfo, Cpuinfo::FEATURE_X86, Cpuinfo::FEATURE_X86_MAX;
print_features $cpuinfo, Cpuinfo::FEATURE_IA64, Cpuinfo::FEATURE_IA64_MAX;
print_features $cpuinfo, Cpuinfo::FEATURE_PPC, Cpuinfo::FEATURE_PPC_MAX;
print_features $cpuinfo, Cpuinfo::FEATURE_MIPS, Cpuinfo::FEATURE_MIPS_MAX;

# Local variables:
# tab-width: 4
# indent-tabs-mode: nil
# End:
