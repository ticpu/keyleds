#
# spec file for package keyleds
#
# Copyright (c) 2017-2020 Julien Hartmann <juli1.hartmann@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself.

Summary: Logitech Keyboard per-key lighting control
Name: keyleds
Version: 1.1.1
Release: 1%{?dist}
License: GPL-3.0+
Group: Applications/System
Source: https://github.com/ticpu/keyleds/archive/v%{version}/%{name}-%{version}.tar.gz
URL: https://github.com/ticpu/keyleds
BuildRequires: cmake, make, gcc, gcc-c++, libudev-devel, libevdev-devel, libuv-devel, libyaml-devel, libX11-devel, libXi-devel, systemd-devel, systemd-rpm-macros
%if 0%{?suse_version}
BuildRequires: lua51-luajit-devel
%else
BuildRequires: luajit-devel
%endif
Requires: udev

%description
Userspace service for Logitech keyboards with per-key RGB LEDs.
The following features are supported:
 - Flexible per-application RGB settings with key groups.
 - Can react to window title changes, switching profiles based on current webpage in browser or open file extension in editors.
 - Native animation plugins: fixed colors, keypress feedback, breathe, wave, stars.
 - Several plugins can be active at once, and composited with alpha blending to build complex effects.
 - Systemd session support, detecting user switches so multiple users can use the service without them fighting over keyboard control.
 - DBus interface for scripting

%prep
%setup -q

%build
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_lib} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXE_LINKER_FLAGS="-Wl,--as-needed -Wl,--no-undefined -Wl,-z,now" -DCMAKE_MODULE_LINKER_FLAGS="-Wl,--as-needed -Wl,-z,now" -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--as-needed -Wl,--no-undefined -Wl,-z,now" -DCMAKE_SKIP_RPATH:BOOL=ON ..
make %{?_smp_mflags}
gzip -9 ../keyledsd/keyledsd.1 -c > keyledsd.1.gz
gzip -9 ../keyledsctl/keyledsctl.1 -c > keyledsctl.1.gz

%install
cd build
%make_install
cd ..
# The udev rule and the systemd user unit are installed by the build itself.
# The autostart entry is gone: it raced with the unit for the session bus name.
install -m 644 -D keyledsd/keyledsd.conf.sample %{buildroot}/%{_sysconfdir}/keyledsd.conf
install -m 644 -D build/keyledsd.1.gz %{buildroot}/%{_mandir}/man1/keyledsd.1
install -m 644 -D build/keyledsctl.1.gz %{buildroot}/%{_mandir}/man1/keyledsctl.1

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc LICENSE
%{_bindir}/*
%{_datadir}/keyledsd
%{_includedir}/*
%{_libdir}/*
%{_libdir}/keyledsd/*
%{_mandir}/man1/*
%config(noreplace) %{_sysconfdir}/keyledsd.conf
%{_userunitdir}/keyledsd.service
%{_udevrulesdir}/70-logitech-hidpp.rules
