%define name	cpuinfo
%define version	0.1
%define release	1
#define svndate	DATE

Summary:		Print CPU information
Name:			%{name}
Version:		%{version}
Release:		%{release}
Source0:		%{name}-%{version}.tar.bz2
License:		GPL
Group:			System/Kernel and hardware
Url:			http://gwenole.beauchesne/projects/cpuinfo
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
cpuinfo gathers some interesting information concerning your CPU. In
particular, it determines useful processor features and cache
hierarchy along with the usual brand and model names.

%prep
%setup -q

%build
mkdir objs
pushd objs
../configure --prefixs=%{_prefix} --libdir=%{_libdir}
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

%changelog
* Mon Apr  9 2007 Gwenole Beauchesne <gb.public@free.fr> 0.1-1
- initial packaging
