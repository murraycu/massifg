Name: massifg
Version: 0.2.1
Release: 1%{?dist}
Summary: A GTK+ based visualizer for valgrinds massif tool
License: GPLv3+
URL: http://gitorious.org/massifg
Group: Development/Tools
Source0: http://www.jonnor.com/files/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: automake,autoconf,libtool,pkgconfig
BuildRequires: gtk2-devel,goffice-devel
Requires: gtk2,goffice

%description
Application that visualizes massif output as a graph.

# Rules
%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

# Tests
%%check
#make check

%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/%{name}/
%{_datadir}/gtk-doc/html/%{name}/
%{_datadir}/applications/%{name}.desktop
%doc README

# Changelog
%changelog
* Fri Sep 3 2010 Jon Nordby <jonn@openimus.com> 0.2-1
- Initial version of the package


