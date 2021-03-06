# $Id: Makefile 3540 2006-03-29 09:36:43Z wbx $

include $(TOPDIR)/rules.mk

PKG_NAME:=openssl
PKG_VERSION:=1.0.2q
PKG_RELEASE:=0
PKG_MD5SUM:=7563e1ce046cb21948eeb6ba1a0eb71c

PKG_SOURCE_URL:=https://dl.meraki.net
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

OPENSSL_NO_CIPHERS:= no-idea no-md2 no-mdc2 no-rc5 no-sha0 no-rmd160 no-ec2m
OPENSSL_OPTIONS:= shared no-err no-fips no-krb5 no-threads zlib-dynamic

ifeq ($(BOARD),$(filter $(BOARD),ar7100 topaz-powerpc venom powerpc))
OPENSSL_NO_CIPHERS += no-camellia no-cast no-seed no-whirlpool no-ripemd160 no-bf
OPENSSL_OPTIONS += no-engine no-gost no-hw no-srp
endif

# tam services requires engine and srp
ifeq ($(BOARD),$(filter $(BOARD),cryptid insect axe))
OPENSSL_NO_CIPHERS += no-camellia no-cast no-seed no-whirlpool no-ripemd160 no-bf
OPENSSL_OPTIONS += no-gost no-hw
endif


ifeq ("$(strip $(ARCH))","x86_64")
	SSL_TARGET := linux-x86_64
	OPENSSL_MAKEFLAGS += LIBDIR=lib
else
	SSL_TARGET := linux-openwrt
endif

include $(TOPDIR)/package/rules.mk

ifeq ($(BOARD),cyclops-2.6)
TARGET_CFLAGS += -O3
endif

$(eval $(call PKG_template,LIBOPENSSL,libopenssl,$(PKG_VERSION)-$(PKG_RELEASE),$(ARCH)))
$(eval $(call PKG_template,OPENSSL_UTIL,openssl-util,$(PKG_VERSION)-$(PKG_RELEASE),$(ARCH)))

$(PKG_BUILD_DIR)/.configured:
	$(SED) 's,/CFLAG=,/CFLAG= $(TARGET_SOFT_FLOAT) ,g' $(PKG_BUILD_DIR)/Configure
	$(SED) s/OPENWRT_OPTIMIZATION_FLAGS/$(BR2_TARGET_OPTIMIZATION)/g $(PKG_BUILD_DIR)/Configure
	(cd $(PKG_BUILD_DIR); \
		PATH=$(TARGET_PATH) \
		./Configure $(SSL_TARGET) \
		  --prefix=/usr \
		  --openssldir=/etc/ssl \
		  -I$(STAGING_DIR)/usr/include \
		  -L$(STAGING_DIR)/usr/lib -ldl \
		  -DOPENSSL_SMALL_FOOTPRINT \
		  $(OPENSSL_NO_CIPHERS) \
		  $(OPENSSL_OPTIONS) \
	)
	touch $@

$(PKG_BUILD_DIR)/.built: $(PKG_BUILD_DIR)/.configured
	rm -rf $(PKG_INSTALL_DIR)
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) -j1 \
		MAKEDEPPROG="$(TARGET_CROSS)gcc $(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		depend
	$(MAKE) -C $(PKG_BUILD_DIR) -j1 \
		CC="$(TARGET_CROSS)gcc $(TARGET_CFLAGS)" \
		AR="$(TARGET_CROSS)ar r" \
		RANLIB="$(TARGET_CROSS)ranlib" \
		$(OPENSSL_MAKEFLAGS) \
		all build-shared
	# Work around openssl build bug to link libssl.so with libcrypto.so.
	-rm $(PKG_BUILD_DIR)/libssl.so.*.*.*
	$(MAKE) -C $(PKG_BUILD_DIR) -j1 \
		CC="$(TARGET_CROSS)gcc $(TARGET_CFLAGS)" \
		CCOPTS="$(TARGET_CFLAGS) -fomit-frame-pointer" \
		$(OPENSSL_MAKEFLAGS) \
		do_linux-shared
	$(MAKE) -C $(PKG_BUILD_DIR) -j1 \
		INSTALL_PREFIX="$(PKG_INSTALL_DIR)" \
		CC="$(TARGET_CROSS)gcc $(TARGET_CFLAGS)" \
		$(OPENSSL_MAKEFLAGS) \
		install
	touch $@

$(IPKG_LIBOPENSSL):
	install -d -m0755 $(IDIR_LIBOPENSSL)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{crypto,ssl}.so.* $(IDIR_LIBOPENSSL)/usr/lib/
	chmod 0644 $(IDIR_LIBOPENSSL)/usr/lib/*
	$(RSTRIP) $(IDIR_LIBOPENSSL)
	$(IPKG_BUILD) $(IDIR_LIBOPENSSL) $(PACKAGE_DIR)

$(IPKG_OPENSSL_UTIL):
	install -d -m0755 $(IDIR_OPENSSL_UTIL)/etc/ssl
	$(CP) $(PKG_INSTALL_DIR)/etc/ssl/openssl.cnf $(IDIR_OPENSSL_UTIL)/etc/ssl/
	install -d -m0755 $(IDIR_OPENSSL_UTIL)/etc/ssl/certs
	install -d -m0755 $(IDIR_OPENSSL_UTIL)/etc/ssl/private
	chmod 0700 $(IDIR_OPENSSL_UTIL)/etc/ssl/private
	install -d -m0755 $(IDIR_OPENSSL_UTIL)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/openssl $(IDIR_OPENSSL_UTIL)/usr/bin/
	$(RSTRIP) $(IDIR_OPENSSL_UTIL)
	$(IPKG_BUILD) $(IDIR_OPENSSL_UTIL) $(PACKAGE_DIR)

$(STAGING_DIR)/usr/lib/libssl.so: $(PKG_BUILD_DIR)/.built
	mkdir -p $(STAGING_DIR)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/openssl $(STAGING_DIR)/usr/include/
	mkdir -p $(STAGING_DIR)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib{crypto,ssl}.{a,so*} $(STAGING_DIR)/usr/lib/
	mkdir -p $(STAGING_DIR)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/lib{crypto,ssl}.pc $(STAGING_DIR)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/openssl.pc $(STAGING_DIR)/usr/lib/pkgconfig
	touch $@

install-dev: $(STAGING_DIR)/usr/lib/libssl.so

uninstall-dev:
	rm -rf \
		$(STAGING_DIR)/usr/include/openssl \
		$(STAGING_DIR)/usr/lib/lib{crypto,ssl}.{a,so*} \
		$(STAGING_DIR)/usr/lib/pkgconfig/lib{crypto,ssl}.pc \
		$(STAGING_DIR)/usr/lib/pkgconfig/openssl.pc \

compile-targets: install-dev
clean-targets: uninstall-dev
