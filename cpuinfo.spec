%define name	cpuinfo
%define version	1.0
%define release	0.1
#define svndate	DATE

Summary:	Print CPU information
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source0:	%{name}-%{version}%{?svndate:-%{svndate}}.tar.bz2
License:	GPL
Group:		System/Kernel and hardware
Url:		http://gwenole.beauchesne/projects/cpuinfo
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
cpuinfo gathers some interesting information concerning your CPU. In
particular, it determines useful processor features and cache
hierarchy along with the usual brand and model names.

%package devel
Summary:	Development files for cpuinfo
Group:		Development/C

%description devel
This package contains headers and libraries needed to use cpuinfo
processor characterisation features.

%prep
%setup -q

%build
mkdir objs
pushd objs
../configure --prefixs=%{_prefix} --libdir=%{_libdir} --install-sdk
make
popd

%install
rm -rf $RPM_BUILD_ROOT

make -C objs install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/cpuinfo

%files devel
%defattr(-,root,root)
%{_includedir}/cpuinfo.h
%{_libdir}/libcpuinfo.a

%changelog
* Fri Apr 13 2007 Gwenole Beauchesne <gb.public@free.fr> 1.0-0.1
- initial packaging
