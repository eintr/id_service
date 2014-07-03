/** \cond 0 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
/** \endcond */

/** \file mod.c
	This is a test module, it reflects every byte of the socket.
*/

#include <room_service.h>

#include "mod_config.h"
struct id_module_config_st id_module_config = {		// Set default values.
	.snd_api_timeout_ms = 1000,
	.rcv_api_timeout_ms = 1000,
};

static int test_config(cJSON *conf)
{
    cJSON *value;

    value = cJSON_GetObjectItem(conf, "Enabled");
    if (value->type != cJSON_True) {
        mylog(L_INFO, "Module is configured disabled.");
        return -1;
    }
	return 0;
}


static int mod_init(cJSON *conf)
{
	if (test_config(conf)<0) {
		return -1;
	}
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

module_interface_t MODULE_INTERFACE_SYMB = 
{
	.mod_initializer = mod_init,
	.mod_destroier = mod_destroy,
	.mod_maintainer = mod_maint,
	.mod_serialize = mod_serialize,
};

