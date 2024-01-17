/**
 * Copyright 2019-2020 DigitalOcean Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>

#include "microhttpd.h"
#include "prom.h"

prom_collector_registry_t *PROM_ACTIVE_REGISTRY;

void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry) {
  if (!active_registry) {
    PROM_ACTIVE_REGISTRY = PROM_COLLECTOR_REGISTRY_DEFAULT;
  } else {
    PROM_ACTIVE_REGISTRY = active_registry;
  }
}

enum MHD_Result promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                     const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
  enum MHD_Result ret;
  struct MHD_Response *response;
  const char *buf;
  enum MHD_ResponseMemoryMode mode;
  unsigned int status_code;

  if (strcmp(method, "GET") != 0) {
    buf = "Invalid HTTP Method\n";
    mode = MHD_RESPMEM_PERSISTENT;
    status_code = MHD_HTTP_BAD_REQUEST;
  } else if (strcmp(url, "/") == 0) {
    buf = "OK\n";
    mode = MHD_RESPMEM_PERSISTENT;
    status_code = MHD_HTTP_OK;
  } else if (strcmp(url, "/metrics") == 0) {
    buf = prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
    mode = MHD_RESPMEM_MUST_FREE;
    status_code = MHD_HTTP_OK;
  } else {
    buf = "Bad Request\n";
    mode = MHD_RESPMEM_PERSISTENT;
    status_code = MHD_HTTP_BAD_REQUEST;
  }

  response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, mode);
  ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);
  return ret;
}

struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
                                         void *apc_cls) {
  return MHD_start_daemon(flags, port, apc, apc_cls, &promhttp_handler, NULL, MHD_OPTION_END);
}
