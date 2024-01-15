:orphan:

.. _migration_2.6:

Migration guide for |NCS| v2.6.0 (Working draft)
################################################

This document describes the changes required or recommended when migrating your application from |NCS| v2.5.0 to |NCS| v2.6.0.

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

* For the Serial LTE Modem (SLM) application:

  * The Zephyr settings backend has been changed from :ref:`FCB <fcb_api>` to :ref:`NVS <nvs_api>`.
    Because of this, one setting is restored to its default value after the switch if you are using the :ref:`liblwm2m_carrier_readme` library.
    The setting controls whether the SLM connects automatically to the network on startup.
    You can read and write it using the ``AT#XCARRIER="auto_connect"`` command.

  * The ``AT#XCMNG`` command storage, activated with :file:`overlay-native_tls.conf` overlay file, has been changed from modem certificate storage to Zephyr settings storage.
    Previously stored credentials will need to be stored again with ``AT#XCMNG`` commands.

* For the Matter samples and applications using Intermittently Connected Devices configuration (formerly called Sleepy End Devices):

  * The naming convention for the energy-optimized devices has been changed from Sleepy End Devices (SED) to Intermittently Connected Devices (ICD).
    Because of this, the Kconfig options used to manage this configuration have been aligned as well.
    If your application uses the following Kconfig options, they require name changes:

      * The ``CONFIG_CHIP_ENABLE_SLEEPY_END_DEVICE_SUPPORT`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ENABLE_ICD_SUPPORT`.
      * The ``CONFIG_CHIP_SED_IDLE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL`.
      * The ``CONFIG_CHIP_SED_ACTIVE_INTERVAL`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_FAST_POLLING_INTERVAL`.
      * The ``CONFIG_CHIP_SED_ACTIVE_THRESHOLD`` Kconfig option was renamed to :kconfig:option:`CONFIG_CHIP_ICD_ACTIVE_MODE_THRESHOLD`.

* For Matter over Thread samples, starting from this release, the cryptography backend enabled by default is PSA Crypto API instead of mbedTLS.
  Be aware of the change and consider the following when migrating to |NCS| v2.6.0:

  * You can keep using mbedTLS API as the cryptography backend by disabling PSA Crypto API.
    You can disable it by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA` Kconfig option to ``n``.
  * When the Device Attestation Certificate (DAC) private key exists in the factory data set, it can migrate to the PSA ITS secure storage.

    You can also have the DAC private key replaced by zeros in the factory data partition by setting the :kconfig:option:`CONFIG_CHIP_CRYPTO_PSA_MIGRATE_DAC_PRIV_KEY` Kconfig option to ``y``.
    This functionality is experimental.

* For samples using Wi-Fi features:

  * A few Kconfig options related to scan operations have been removed in the current release.

     If your application uses scan operations, they need to be updated to remove the dependency on the following options:

      * ``CONFIG_WIFI_MGMT_SCAN_BANDS``
      * ``CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_ACTIVE``
      * ``CONFIG_WIFI_MGMT_SCAN_DWELL_TIME_PASSIVE``
      * ``CONFIG_WIFI_MGMT_SCAN_SSID_FILT``
      * ``CONFIG_WIFI_MGMT_SCAN_MAX_BSS_CNT``
      * ``CONFIG_WIFI_MGMT_SCAN_CHAN``

  * Instead of the ``CONFIG_WIFI_MGMT_SCAN_MAX_BSS_CNT`` Kconfig option, a new :kconfig:option:`CONFIG_NRF_WIFI_SCAN_MAX_BSS_CNT` Kconfig option is added.

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

.. HOWTO

   Add changes in the following format:

.. * Change1 and description
.. * Change2 and description
