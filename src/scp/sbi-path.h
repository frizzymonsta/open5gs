/*
 * Copyright (C) 2019-2022 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCP_SBI_PATH_H
#define SCP_SBI_PATH_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

int scp_sbi_open(void);
void scp_sbi_close(void);

bool scp_sbi_send_request(
        ogs_sbi_object_t *sbi_object,
        ogs_sbi_service_type_e service_type,
        void *data);
bool scp_sbi_discover_and_send(
        ogs_sbi_service_type_e service_type,
        ogs_sbi_discovery_option_t *discovery_option,
        ogs_sbi_request_t *(*build)(scp_conn_t *conn, void *data),
        scp_conn_t *conn, ogs_sbi_stream_t *stream, void *data);

#ifdef __cplusplus
}
#endif

#endif /* SCP_SBI_PATH_H */
