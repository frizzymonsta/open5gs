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

#ifndef OGS_NNRF_BUILD_H
#define OGS_NNRF_BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

OpenAPI_nf_profile_t *ogs_nnrf_nfm_build_nf_profile(
        ogs_sbi_nf_instance_t *nf_instance,
        ogs_sbi_discovery_option_t *discovery_option,
        uint64_t supported_features);
void ogs_nnrf_nfm_free_nf_profile(OpenAPI_nf_profile_t *NFProfile);

ogs_sbi_request_t *ogs_nnrf_nfm_build_register(void);
ogs_sbi_request_t *ogs_nnrf_nfm_build_update(void);
ogs_sbi_request_t *ogs_nnrf_nfm_build_de_register(void);

ogs_sbi_request_t *ogs_nnrf_nfm_build_status_subscribe(
        ogs_sbi_subscription_t *subscription);
ogs_sbi_request_t *ogs_nnrf_nfm_build_status_unsubscribe(
        ogs_sbi_subscription_t *subscription);
ogs_sbi_request_t *ogs_nnrf_nfm_build_profile_retrieve(char *nf_instance_id);

ogs_sbi_request_t *ogs_nnrf_disc_build_discover(
        ogs_sbi_service_type_e service_type,
        ogs_sbi_discovery_option_t *discovery_option);

#ifdef __cplusplus
}
#endif

#endif /* OGS_NNRF_BUILD_H */
