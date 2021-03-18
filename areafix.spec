%global ver_major 1
%global ver_minor 9
%global ver_patch 0
%global reldate 20210318
%global reltype C
# may be one of: C (current), R (release), S (stable)

# release number for Release: header
%global relnum 4

# on default static library is made but using 'rpmbuild --without static'
# produces a dynamic library
%if "%_vendor" == "alt"
    %def_with static
%else
    %bcond_without static
%endif

# if you use 'rpmbuild --with debug' then debug binary is produced
%if "%_vendor" == "alt"
    %def_without debug
%else
    %bcond_with debug
%endif

%global debug_package %nil

# for generic build; will override for some distributions
%global vendor_prefix %nil
%global vendor_suffix %nil
%global pkg_group Applications/Communications

# for CentOS, Fedora and RHEL
%if "%_vendor" == "redhat"
    %global vendor_suffix %dist
%endif

# for ALT Linux
%if "%_vendor" == "alt"
    %global vendor_prefix %_vendor
    %global pkg_group Networking/FTN
%endif

%global main_name areafix
%if %{with static}
Name: %main_name-static
%else
Name: %main_name
%endif
Version: %ver_major.%ver_minor.%reldate%reltype
Release: %{vendor_prefix}%relnum%{vendor_suffix}
%if "%_vendor" != "redhat"
Group: %pkg_group
%endif
%if %{with static}
Summary: Static areafix library for the Husky Project software
%else
Summary: Dynamic areafix library for the Husky Project software
%endif
URL: https://github.com/huskyproject/%main_name/archive/v%ver_major.%ver_minor.%reldate.tar.gz
License: LGPLv2
Source: %main_name-%ver_major.%ver_minor.%reldate.tar.gz
BuildRequires: gcc
%if %{with static}
BuildRequires: huskylib-static huskylib-static-devel
BuildRequires: smapi-static smapi-static-devel
BuildRequires: fidoconf-static fidoconf-static-devel
%else
BuildRequires: huskylib huskylib-devel
BuildRequires: smapi smapi-devel
BuildRequires: fidoconf fidoconf-devel
Requires: huskylib smapi fidoconf
%endif
BuildRequires: perl >= 5.8.8, perl-devel >= 5.8.8
Requires: perl >= 5.8.8

%description
%summary


%package devel
%if "%_vendor" != "redhat"
Group: %pkg_group
%endif
Summary: Development headers for %main_name
%if %{with static}
BuildArch: noarch
%else
Requires: %name = %version-%release
%endif
%description devel
%summary

%prep
%setup -q -n %main_name-%ver_major.%ver_minor.%reldate

%build
%if %{with static}
    %if %{with debug}
        %make_build DEBUG=1
    %else
        %make_build
    %endif
%else
    %if %{with debug}
        %make_build DYNLIBS=1 DEBUG=1
    %else
        %make_build DYNLIBS=1
    %endif
%endif

%if %{with debug}
    %global __os_install_post %nil
%endif

%install
umask 022
%if %{with static}
    %if %{with debug}
        %make_install DEBUG=1
    %else
        %make_install
    %endif
%else
    %if %{with debug}
        %make_install DYNLIBS=1 DEBUG=1
    %else
        %make_install DYNLIBS=1
    %endif
%endif
chmod -R a+rX,u+w,go-w %buildroot

%files
%defattr(-,root,root)
%if %{with static}
    %_libdir/*.a
%else
    %exclude %_libdir/*.a
    %_libdir/*.so.%ver_major.%ver_minor.%ver_patch
    %_libdir/*.so.%ver_major.%ver_minor
%endif

%files devel
%dir %_includedir/%main_name
%_includedir/%main_name/*
%if ! %{with static}
    %_libdir/*.so
%endif
