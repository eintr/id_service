/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/** \endcond */

/** \file mod.c
	This is a test module, it reflects every byte of the socket.
*/

#include "id_api_listener.h"

#include "mod_config.h"
struct id_module_config_st id_module_config = {		// Set default values.
	.snd_api_timeout_ms = 1000,
	.rcv_api_timeout_ms = 1000,

	.id_config_dir = "/",
};

static int mod_init(cJSON *conf)
{
	api_interface.mod_initializer(conf);
	return 0;
}

static int mod_destroy(void)
{
	api_interface.mod_destroier();
	return 0;
}

static void mod_maint(void)
{
}

static cJSON *mod_serialize(void)
{
	return NULL;
}

