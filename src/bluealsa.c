/*
 * BlueALSA - bluealsa.c
 * Copyright (c) 2016-2019 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "bluealsa.h"

#include <fcntl.h>
#include <grp.h>
#include <poll.h>

#if ENABLE_LDAC
# include <ldacBT.h>
#endif

#include "hfp.h"
#include "transport.h"


/* Initialize global configuration variable. */
struct ba_config config = {

	/* enable output profiles by default */
	.enable.a2dp_source = true,
	.enable.hfp_ag = true,
	.enable.hsp_ag = true,

	.null_fd = -1,

	/* omit chown if audio group is not defined */
	.gid_audio = -1,

	.hfp.features_sdp_hf =
		SDP_HFP_HF_FEAT_CLI |
		SDP_HFP_HF_FEAT_VOLUME,
	.hfp.features_sdp_ag = 0,
	.hfp.features_rfcomm_hf =
		HFP_HF_FEAT_CLI |
		HFP_HF_FEAT_VOLUME |
		HFP_HF_FEAT_ECS |
		HFP_HF_FEAT_ECC |
		HFP_HF_FEAT_CODEC,
	.hfp.features_rfcomm_ag =
		HFP_AG_FEAT_REJECT |
		HFP_AG_FEAT_ECS |
		HFP_AG_FEAT_ECC |
		HFP_AG_FEAT_EERC |
		HFP_AG_FEAT_CODEC,

	.a2dp.volume = false,
	.a2dp.force_mono = false,
	.a2dp.force_44100 = false,
	.a2dp.keep_alive = 0,

	.a2dp.direct_fifo = FIFO_OFF,
	.a2dp.direct_fifo_inband = false,

#if ENABLE_AAC
	/* There are two issues with the afterburner: a) it uses a LOT of power,
	 * b) it generates larger payload. These two reasons are good enough to
	 * not enable afterburner by default. */
	.aac_afterburner = false,
	.aac_vbr_mode = 4,
#endif

#if ENABLE_LDAC
	.ldac_abr = false,
	/* Use standard encoder quality as a reasonable default. */
	.ldac_eqmid = LDACBT_EQMID_SQ,
#endif

};

int bluealsa_config_init(void) {

	struct group *grp;

	config.main_thread = pthread_self();

	pthread_mutex_init(&config.devices_mutex, NULL);
	config.devices = g_hash_table_new_full(g_str_hash, g_str_equal,
			g_free, (GDestroyNotify)device_free);

	config.dbus_objects = g_hash_table_new_full(g_direct_hash, g_direct_equal,
			NULL, g_free);

	config.null_fd = open("/dev/null", O_WRONLY | O_NONBLOCK);

	/* use proper ACL group for our audio device */
	if ((grp = getgrnam("audio")) != NULL)
		config.gid_audio = grp->gr_gid;

	config.a2dp.codecs = bluez_a2dp_codecs;

	return 0;
}
