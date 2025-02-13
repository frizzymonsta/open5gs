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

#include "ogs-sbi.h"

static OpenAPI_nf_service_t *build_nf_service(
        ogs_sbi_nf_service_t *nf_service);
static void free_nf_service(OpenAPI_nf_service_t *NFService);
static OpenAPI_smf_info_t *build_smf_info(ogs_sbi_nf_info_t *nf_info);
static void free_smf_info(OpenAPI_smf_info_t *SmfInfo);

ogs_sbi_request_t *ogs_nnrf_nfm_build_register(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;

    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    OpenAPI_nf_profile_t *NFProfile = NULL;
    uint64_t supported_features = 0;

    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_assert(nf_instance->id);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PUT;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_NF_INSTANCES;
    message.h.resource.component[1] = nf_instance->id;

    message.http.content_encoding = (char*)ogs_sbi_self()->content_encoding;

    OGS_SBI_FEATURES_SET(supported_features, OGS_SBI_NNRF_NFM_SERVICE_MAP);
    NFProfile = ogs_nnrf_nfm_build_nf_profile(
                    ogs_sbi_self()->nf_instance, NULL, supported_features);
    ogs_expect_or_return_val(NFProfile, NULL);

    message.NFProfile = NFProfile;

    request = ogs_sbi_build_request(&message);

    ogs_nnrf_nfm_free_nf_profile(NFProfile);

    return request;
}

OpenAPI_nf_profile_t *ogs_nnrf_nfm_build_nf_profile(
        ogs_sbi_nf_instance_t *nf_instance,
        ogs_sbi_discovery_option_t *discovery_option,
        uint64_t supported_features)
{
    ogs_sbi_nf_service_t *nf_service = NULL;
    ogs_sbi_nf_info_t *nf_info = NULL;

    OpenAPI_nf_profile_t *NFProfile = NULL;
    OpenAPI_list_t *Ipv4AddrList = NULL;
    OpenAPI_list_t *Ipv6AddrList = NULL;
    OpenAPI_list_t *AllowedNfTypeList = NULL;
    OpenAPI_list_t *NFServiceList = NULL;
    OpenAPI_map_t *NFServiceMap = NULL;
    OpenAPI_list_t *SmfInfoList = NULL;
    OpenAPI_map_t *SmfInfoMap = NULL;
    OpenAPI_smf_info_t *SmfInfo = NULL;
    int SmfInfoMapKey;

    OpenAPI_lnode_t *node = NULL;

    int i = 0;
    char *ipstr = NULL;

    ogs_assert(nf_instance);
    ogs_assert(nf_instance->id);

    NFProfile = ogs_calloc(1, sizeof(*NFProfile));
    ogs_expect_or_return_val(NFProfile, NULL);

	NFProfile->nf_instance_id = nf_instance->id;
	NFProfile->nf_type = nf_instance->nf_type;
	NFProfile->nf_status = nf_instance->nf_status;

    ogs_trace("[%s] ogs_nnrf_nfm_build_nf_profile()", nf_instance->id);

    ogs_trace("NF-Type[%s] NF-Status[%s] IPv4[%d] IPv6[%d]",
                OpenAPI_nf_type_ToString(nf_instance->nf_type),
                OpenAPI_nf_status_ToString(nf_instance->nf_status),
                nf_instance->num_of_ipv4, nf_instance->num_of_ipv6);

    if (nf_instance->time.heartbeat_interval) {
        NFProfile->is_heart_beat_timer = true;
        NFProfile->heart_beat_timer = nf_instance->time.heartbeat_interval;
    }
    NFProfile->is_nf_profile_changes_support_ind = true;
    NFProfile->nf_profile_changes_support_ind = true;

    if (nf_instance->fqdn)
        NFProfile->fqdn = ogs_strdup(nf_instance->fqdn);

    NFProfile->is_priority = true;
    NFProfile->priority = nf_instance->priority;
    NFProfile->is_capacity = true;
    NFProfile->capacity = nf_instance->capacity;
    NFProfile->is_load = true;
    NFProfile->load = nf_instance->load;

    Ipv4AddrList = OpenAPI_list_create();
    ogs_assert(Ipv4AddrList);
    Ipv6AddrList = OpenAPI_list_create();
    ogs_assert(Ipv6AddrList);

    for (i = 0; i < nf_instance->num_of_ipv4; i++) {
        if (nf_instance->ipv4[i]) {
            ogs_trace("IPv4 [family:%d, addr:%x, port:%d]",
                    nf_instance->ipv4[i]->ogs_sa_family,
                    htobe32(nf_instance->ipv4[i]->sin.sin_addr.s_addr),
                    nf_instance->ipv4[i]->ogs_sin_port);
            ogs_assert(nf_instance->ipv4[i]->ogs_sa_family == AF_INET);
            ipstr = ogs_ipstrdup(nf_instance->ipv4[i]);
            ogs_expect_or_return_val(ipstr, NULL);
            OpenAPI_list_add(Ipv4AddrList, ipstr);
        }
    }
    for (i = 0; i < nf_instance->num_of_ipv6; i++) {
        if (nf_instance->ipv6[i]) {
            ogs_trace("IPv6 [family:%d, port:%d]",
                    nf_instance->ipv6[i]->ogs_sa_family,
                    nf_instance->ipv6[i]->ogs_sin_port);
            ogs_assert(nf_instance->ipv6[i]->ogs_sa_family == AF_INET6);
            ipstr = ogs_ipstrdup(nf_instance->ipv6[i]);
            ogs_expect_or_return_val(ipstr, NULL);
            OpenAPI_list_add(Ipv6AddrList, ipstr);
        }
    }

    if (Ipv4AddrList->count)
        NFProfile->ipv4_addresses = Ipv4AddrList;
    else
        OpenAPI_list_free(Ipv4AddrList);
    if (Ipv6AddrList->count)
        NFProfile->ipv6_addresses = Ipv6AddrList;
    else
        OpenAPI_list_free(Ipv6AddrList);

    AllowedNfTypeList = OpenAPI_list_create();
    ogs_assert(AllowedNfTypeList);

    for (i = 0; i < nf_instance->num_of_allowed_nf_type; i++) {
        OpenAPI_list_add(AllowedNfTypeList,
                (void *)(uintptr_t)nf_instance->allowed_nf_type[i]);
    }

    if (AllowedNfTypeList->count)
        NFProfile->allowed_nf_types = AllowedNfTypeList;
    else
        OpenAPI_list_free(AllowedNfTypeList);

    NFServiceList = OpenAPI_list_create();
    ogs_assert(NFServiceList);

    ogs_list_for_each(&nf_instance->nf_service_list, nf_service) {
        OpenAPI_nf_service_t *NFService = NULL;

        if (discovery_option && discovery_option->num_of_service_names) {
            for (i = 0; i < discovery_option->num_of_service_names; i++) {
                if (nf_service->name &&
                    discovery_option->service_names[i] &&
                    strcmp(nf_service->name,
                        discovery_option->service_names[i]) == 0) {
                    break;
                }
            }

            if (i == discovery_option->num_of_service_names)
                continue;
        }

        NFService = build_nf_service(nf_service);
        ogs_expect_or_return_val(NFService, NULL);

        if (OGS_SBI_FEATURES_IS_SET(
            supported_features, OGS_SBI_NNRF_NFM_SERVICE_MAP)) {
            NFServiceMap = OpenAPI_map_create(nf_service->id, NFService);
            ogs_assert(NFServiceMap);

            OpenAPI_list_add(NFServiceList, NFServiceMap);
        } else {
            OpenAPI_list_add(NFServiceList, NFService);
        }
    }

    if (NFServiceList->count) {
        if (OGS_SBI_FEATURES_IS_SET(
            supported_features, OGS_SBI_NNRF_NFM_SERVICE_MAP)) {
            NFProfile->nf_service_list = NFServiceList;
        } else {
            NFProfile->nf_services = NFServiceList;
        }
    } else
        OpenAPI_list_free(NFServiceList);

    SmfInfoList = OpenAPI_list_create();
    ogs_assert(SmfInfoList);

    SmfInfoMapKey = 0;

    ogs_list_for_each(&nf_instance->nf_info_list, nf_info) {
        if (nf_info->nf_type == OpenAPI_nf_type_SMF) {

            if (nf_info->smf.num_of_slice == 0) {
                ogs_fatal("CHECK CONFIGURATION: No S-NSSAI");
                ogs_assert_if_reached();
            }

            SmfInfo = build_smf_info(nf_info);
            ogs_expect_or_return_val(SmfInfo, NULL);

            SmfInfoMap = OpenAPI_map_create(
                    ogs_msprintf("%d", ++SmfInfoMapKey), SmfInfo);
            ogs_assert(SmfInfoMap);

            OpenAPI_list_add(SmfInfoList, SmfInfoMap);

        } else {
            ogs_fatal("Not implemented NF-type[%s]",
                    OpenAPI_nf_type_ToString(nf_info->nf_type));
            ogs_assert_if_reached();
        }
    }

    if (SmfInfoList->count == 1) {
        NFProfile->smf_info = SmfInfo;

        OpenAPI_list_for_each(SmfInfoList, node) {
            SmfInfoMap = node->data;
            if (SmfInfoMap) {
                if (SmfInfoMap->key)
                    ogs_free(SmfInfoMap->key);
                ogs_free(SmfInfoMap);
            }
        }
        OpenAPI_list_free(SmfInfoList);
    } else if (SmfInfoList->count > 1) {
        NFProfile->smf_info_list = SmfInfoList;
    } else
        OpenAPI_list_free(SmfInfoList);

    return NFProfile;
}

void ogs_nnrf_nfm_free_nf_profile(OpenAPI_nf_profile_t *NFProfile)
{
    OpenAPI_map_t *NFServiceMap = NULL;
    OpenAPI_nf_service_t *NFService = NULL;
    OpenAPI_map_t *SmfInfoMap = NULL;
    OpenAPI_smf_info_t *SmfInfo = NULL;
    OpenAPI_lnode_t *node = NULL;

    ogs_assert(NFProfile);

    if (NFProfile->fqdn)
        ogs_free(NFProfile->fqdn);

    OpenAPI_list_for_each(NFProfile->ipv4_addresses, node)
        ogs_free(node->data);
    OpenAPI_list_free(NFProfile->ipv4_addresses);
    OpenAPI_list_for_each(NFProfile->ipv6_addresses, node)
        ogs_free(node->data);
    OpenAPI_list_free(NFProfile->ipv6_addresses);

    OpenAPI_list_free(NFProfile->allowed_nf_types);

    OpenAPI_list_for_each(NFProfile->nf_services, node) {
        NFService = node->data;
        ogs_assert(NFService);
        free_nf_service(NFService);
    }
    OpenAPI_list_free(NFProfile->nf_services);

    OpenAPI_list_for_each(NFProfile->nf_service_list, node) {
        NFServiceMap = node->data;
        if (NFServiceMap) {
            NFService = NFServiceMap->value;
            ogs_assert(NFService);
            free_nf_service(NFService);
            ogs_free(NFServiceMap);
        }
    }
    OpenAPI_list_free(NFProfile->nf_service_list);

    OpenAPI_list_for_each(NFProfile->smf_info_list, node) {
        SmfInfoMap = node->data;
        if (SmfInfoMap) {
            SmfInfo = SmfInfoMap->value;
            if (SmfInfo)
                free_smf_info(SmfInfo);
            if (SmfInfoMap->key)
                ogs_free(SmfInfoMap->key);
            ogs_free(SmfInfoMap);
        }
    }
    OpenAPI_list_free(NFProfile->smf_info_list);

    if (NFProfile->smf_info)
        free_smf_info(NFProfile->smf_info);

    ogs_free(NFProfile);
}

static OpenAPI_nf_service_t *build_nf_service(
        ogs_sbi_nf_service_t *nf_service)
{
    int i;
    OpenAPI_nf_service_t *NFService = NULL;
    OpenAPI_list_t *VersionList = NULL;
    OpenAPI_list_t *IpEndPointList = NULL;
    OpenAPI_list_t *AllowedNfTypeList = NULL;

    ogs_assert(nf_service);
    ogs_assert(nf_service->id);
    ogs_assert(nf_service->name);

    NFService = ogs_calloc(1, sizeof(*NFService));
    ogs_expect_or_return_val(NFService, NULL);
    NFService->service_instance_id = ogs_strdup(nf_service->id);
    ogs_expect_or_return_val(NFService->service_instance_id, NULL);
    NFService->service_name = ogs_strdup(nf_service->name);
    ogs_expect_or_return_val(NFService->service_name, NULL);

    VersionList = OpenAPI_list_create();
    ogs_assert(VersionList);

    for (i = 0; i < nf_service->num_of_version; i++) {
        OpenAPI_nf_service_version_t *NFServiceVersion = NULL;

        NFServiceVersion = ogs_calloc(1, sizeof(*NFServiceVersion));
        ogs_expect_or_return_val(NFServiceVersion, NULL);
        if (nf_service->version[i].in_uri) {
            NFServiceVersion->api_version_in_uri =
                ogs_strdup(nf_service->version[i].in_uri);
            ogs_expect_or_return_val(
                NFServiceVersion->api_version_in_uri, NULL);
        }
        if (nf_service->version[i].full) {
            NFServiceVersion->api_full_version =
                ogs_strdup(nf_service->version[i].full);
            ogs_expect_or_return_val(
                NFServiceVersion->api_full_version, NULL);
        }
        if (nf_service->version[i].expiry) {
            NFServiceVersion->expiry =
                ogs_strdup(nf_service->version[i].expiry);
            ogs_expect_or_return_val(
                NFServiceVersion->expiry, NULL);
        }

        OpenAPI_list_add(VersionList, NFServiceVersion);
    }

    ogs_assert(VersionList->count);
    NFService->versions = VersionList;

    NFService->scheme = nf_service->scheme;
    NFService->nf_service_status = nf_service->status;

    if (nf_service->fqdn)
        NFService->fqdn = ogs_strdup(nf_service->fqdn);

    IpEndPointList = OpenAPI_list_create();
    ogs_assert(IpEndPointList);

    for (i = 0; i < nf_service->num_of_addr; i++) {
        ogs_sockaddr_t *ipv4 = NULL;
        ogs_sockaddr_t *ipv6 = NULL;

        OpenAPI_ip_end_point_t *IpEndPoint = NULL;

        ipv4 = nf_service->addr[i].ipv4;
        ipv6 = nf_service->addr[i].ipv6;

        if (ipv4 || ipv6) {
            IpEndPoint = ogs_calloc(1, sizeof(*IpEndPoint));
            ogs_expect_or_return_val(IpEndPoint, NULL);
            if (ipv4) {
                IpEndPoint->ipv4_address = ogs_ipstrdup(ipv4);
                ogs_expect_or_return_val(IpEndPoint->ipv4_address, NULL);
            }
            if (ipv6) {
                IpEndPoint->ipv6_address = ogs_ipstrdup(ipv6);
                ogs_expect_or_return_val(IpEndPoint->ipv6_address, NULL);
            }
            IpEndPoint->is_port = true;
            IpEndPoint->port = nf_service->addr[i].port;
            OpenAPI_list_add(IpEndPointList, IpEndPoint);
        }
    }

    if (IpEndPointList->count)
        NFService->ip_end_points = IpEndPointList;
    else
        OpenAPI_list_free(IpEndPointList);

    AllowedNfTypeList = OpenAPI_list_create();
    ogs_assert(AllowedNfTypeList);

    for (i = 0; i < nf_service->num_of_allowed_nf_type; i++) {
        OpenAPI_list_add(AllowedNfTypeList,
                (void *)(uintptr_t)nf_service->allowed_nf_type[i]);
    }

    if (AllowedNfTypeList->count)
        NFService->allowed_nf_types = AllowedNfTypeList;
    else
        OpenAPI_list_free(AllowedNfTypeList);

    NFService->is_priority = true;
    NFService->priority = nf_service->priority;
    NFService->is_capacity = true;
    NFService->capacity = nf_service->capacity;
    NFService->is_load = true;
    NFService->load = nf_service->load;

    return NFService;
}

static void free_nf_service(OpenAPI_nf_service_t *NFService)
{
    OpenAPI_lnode_t *node = NULL;

    ogs_assert(NFService);

    ogs_free(NFService->service_instance_id);
    ogs_free(NFService->service_name);

    OpenAPI_list_for_each(NFService->versions, node) {
        OpenAPI_nf_service_version_t *NFServiceVersion = node->data;
        ogs_assert(NFServiceVersion);
        ogs_free(NFServiceVersion->api_version_in_uri);
        ogs_free(NFServiceVersion->api_full_version);
        if (NFServiceVersion->expiry)
            ogs_free(NFServiceVersion->expiry);
        ogs_free(NFServiceVersion);
    }
    OpenAPI_list_free(NFService->versions);

    OpenAPI_list_for_each(NFService->ip_end_points, node) {
        OpenAPI_ip_end_point_t *IpEndPoint = node->data;
        ogs_assert(IpEndPoint);
        if (IpEndPoint->ipv4_address)
            ogs_free(IpEndPoint->ipv4_address);
        if (IpEndPoint->ipv6_address)
            ogs_free(IpEndPoint->ipv6_address);
        ogs_free(IpEndPoint);
    }
    OpenAPI_list_free(NFService->ip_end_points);

    OpenAPI_list_free(NFService->allowed_nf_types);

    if (NFService->fqdn)
        ogs_free(NFService->fqdn);

    ogs_free(NFService);
}

static OpenAPI_smf_info_t *build_smf_info(ogs_sbi_nf_info_t *nf_info)
{
    int i, j;
    OpenAPI_smf_info_t *SmfInfo = NULL;

    OpenAPI_list_t *sNssaiSmfInfoList = NULL;
    OpenAPI_snssai_smf_info_item_t *sNssaiSmfInfoItem = NULL;
    OpenAPI_snssai_t *sNssai = NULL;
    OpenAPI_list_t *DnnSmfInfoList = NULL;
    OpenAPI_dnn_smf_info_item_t *DnnSmfInfoItem = NULL;

    OpenAPI_list_t *TaiList = NULL;
    OpenAPI_tai_t *TaiItem = NULL;
    OpenAPI_list_t *TaiRangeList = NULL;
    OpenAPI_tai_range_t *TaiRangeItem = NULL;
    OpenAPI_list_t *TacRangeList = NULL;
    OpenAPI_tac_range_t *TacRangeItem = NULL;

    ogs_assert(nf_info);

    SmfInfo = ogs_calloc(1, sizeof(*SmfInfo));
    ogs_expect_or_return_val(SmfInfo, NULL);

    sNssaiSmfInfoList = OpenAPI_list_create();
    ogs_assert(sNssaiSmfInfoList);

    for (i = 0; i < nf_info->smf.num_of_slice; i++) {
        DnnSmfInfoList = OpenAPI_list_create();
        ogs_assert(DnnSmfInfoList);

        for (j = 0; j < nf_info->smf.slice[i].num_of_dnn; j++) {
            DnnSmfInfoItem = ogs_calloc(1, sizeof(*DnnSmfInfoItem));
            ogs_expect_or_return_val(DnnSmfInfoItem, NULL);
            DnnSmfInfoItem->dnn = nf_info->smf.slice[i].dnn[j];

            OpenAPI_list_add(DnnSmfInfoList, DnnSmfInfoItem);
        }

        if (!DnnSmfInfoList->count) {
            OpenAPI_list_free(DnnSmfInfoList);

            ogs_error("CHECK CONFIGURATION: No DNN");
            ogs_expect_or_return_val(0, NULL);
        }

        sNssaiSmfInfoItem = ogs_calloc(1, sizeof(*sNssaiSmfInfoItem));
        ogs_expect_or_return_val(sNssaiSmfInfoItem, NULL);

        sNssaiSmfInfoItem->dnn_smf_info_list = DnnSmfInfoList;

        sNssaiSmfInfoItem->s_nssai = sNssai =
            ogs_calloc(1, sizeof(*sNssai));
        ogs_expect_or_return_val(sNssai, NULL);
        sNssai->sst = nf_info->smf.slice[i].s_nssai.sst;
        sNssai->sd =
            ogs_s_nssai_sd_to_string(nf_info->smf.slice[i].s_nssai.sd);

        OpenAPI_list_add(sNssaiSmfInfoList, sNssaiSmfInfoItem);
    }

    if (sNssaiSmfInfoList->count)
        SmfInfo->s_nssai_smf_info_list = sNssaiSmfInfoList;
    else
        OpenAPI_list_free(sNssaiSmfInfoList);

    TaiList = OpenAPI_list_create();
    ogs_assert(TaiList);

    for (i = 0; i < nf_info->smf.num_of_nr_tai; i++) {
        TaiItem = ogs_calloc(1, sizeof(*TaiItem));
        ogs_expect_or_return_val(TaiItem, NULL);
        TaiItem->plmn_id = ogs_sbi_build_plmn_id(
                &nf_info->smf.nr_tai[i].plmn_id);
        ogs_expect_or_return_val(TaiItem->plmn_id, NULL);
        TaiItem->tac =
            ogs_uint24_to_0string(nf_info->smf.nr_tai[i].tac);
        ogs_expect_or_return_val(TaiItem->tac, NULL);

        OpenAPI_list_add(TaiList, TaiItem);
    }

    if (TaiList->count)
        SmfInfo->tai_list = TaiList;
    else
        OpenAPI_list_free(TaiList);

    TaiRangeList = OpenAPI_list_create();
    ogs_assert(TaiRangeList);

    for (i = 0; i < nf_info->smf.num_of_nr_tai_range; i++) {
        TacRangeList = OpenAPI_list_create();
        ogs_assert(TacRangeList);

        for (j = 0;
                j < nf_info->smf.nr_tai_range[i].num_of_tac_range;
                j++) {
            TacRangeItem = ogs_calloc(1, sizeof(*TacRangeItem));
            ogs_expect_or_return_val(TacRangeItem, NULL);

            TacRangeItem->start = ogs_uint24_to_0string(
                    nf_info->smf.nr_tai_range[i].start[j]);
            ogs_expect_or_return_val(TacRangeItem->start, NULL);
            TacRangeItem->end =
                ogs_uint24_to_0string(
                        nf_info->smf.nr_tai_range[i].end[j]);
            ogs_expect_or_return_val(TacRangeItem->end, NULL);

            OpenAPI_list_add(TacRangeList, TacRangeItem);
        }

        if (!TacRangeList->count) {
            OpenAPI_list_free(TacRangeList);

            ogs_error("CHECK CONFIGURATION: No Start/End in TacRange");
            ogs_expect_or_return_val(0, NULL);
        }

        TaiRangeItem = ogs_calloc(1, sizeof(*TaiRangeItem));
        ogs_expect_or_return_val(TaiRangeItem, NULL);

        TaiRangeItem->plmn_id = ogs_sbi_build_plmn_id(
                &nf_info->smf.nr_tai_range[i].plmn_id);
        ogs_expect_or_return_val(TaiRangeItem->plmn_id, NULL);

        TaiRangeItem->tac_range_list = TacRangeList;

        OpenAPI_list_add(TaiRangeList, TaiRangeItem);
    }

    if (TaiRangeList->count)
        SmfInfo->tai_range_list = TaiRangeList;
    else
        OpenAPI_list_free(TaiRangeList);

    return SmfInfo;
}

static void free_smf_info(OpenAPI_smf_info_t *SmfInfo)
{
    OpenAPI_list_t *sNssaiSmfInfoList = NULL;
    OpenAPI_snssai_smf_info_item_t *sNssaiSmfInfoItem = NULL;
    OpenAPI_snssai_t *sNssai = NULL;
    OpenAPI_list_t *DnnSmfInfoList = NULL;
    OpenAPI_dnn_smf_info_item_t *DnnSmfInfoItem = NULL;

    OpenAPI_list_t *TaiList = NULL;
    OpenAPI_tai_t *TaiItem = NULL;
    OpenAPI_list_t *TaiRangeList = NULL;
    OpenAPI_tai_range_t *TaiRangeItem = NULL;
    OpenAPI_list_t *TacRangeList = NULL;
    OpenAPI_tac_range_t *TacRangeItem = NULL;

    OpenAPI_lnode_t *node = NULL, *node2 = NULL;

    ogs_assert(SmfInfo);

    sNssaiSmfInfoList = SmfInfo->s_nssai_smf_info_list;
    OpenAPI_list_for_each(sNssaiSmfInfoList, node) {
        sNssaiSmfInfoItem = node->data;
        ogs_assert(sNssaiSmfInfoItem);

        DnnSmfInfoList = sNssaiSmfInfoItem->dnn_smf_info_list;
        OpenAPI_list_for_each(DnnSmfInfoList, node2) {
            DnnSmfInfoItem = node2->data;
            ogs_assert(DnnSmfInfoItem);
            ogs_free(DnnSmfInfoItem);
        }
        OpenAPI_list_free(DnnSmfInfoList);

        sNssai = sNssaiSmfInfoItem->s_nssai;
        if (sNssai) {
            if (sNssai->sd)
                ogs_free(sNssai->sd);
            ogs_free(sNssai);
        }

        ogs_free(sNssaiSmfInfoItem);
    }
    OpenAPI_list_free(sNssaiSmfInfoList);

    TaiList = SmfInfo->tai_list;
    OpenAPI_list_for_each(TaiList, node) {
        TaiItem = node->data;
        ogs_assert(TaiItem);
        if (TaiItem->plmn_id)
            ogs_sbi_free_plmn_id(TaiItem->plmn_id);
        if (TaiItem->tac)
            ogs_free(TaiItem->tac);
        ogs_free(TaiItem);
    }
    OpenAPI_list_free(TaiList);

    TaiRangeList = SmfInfo->tai_range_list;
    OpenAPI_list_for_each(TaiRangeList, node) {
        TaiRangeItem = node->data;
        ogs_assert(TaiRangeItem);

        if (TaiRangeItem->plmn_id)
            ogs_sbi_free_plmn_id(TaiRangeItem->plmn_id);

        TacRangeList = TaiRangeItem->tac_range_list;
        OpenAPI_list_for_each(TacRangeList, node2) {
            TacRangeItem = node2->data;
            ogs_assert(TacRangeItem);
            if (TacRangeItem->start)
                ogs_free(TacRangeItem->start);
            if (TacRangeItem->end)
                ogs_free(TacRangeItem->end);

            ogs_free(TacRangeItem);
        }
        OpenAPI_list_free(TacRangeList);

        ogs_free(TaiRangeItem);
    }
    OpenAPI_list_free(TaiRangeList);

    ogs_free(SmfInfo);
}

ogs_sbi_request_t *ogs_nnrf_nfm_build_update(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;

    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    OpenAPI_list_t *PatchItemList;
    OpenAPI_patch_item_t item;

    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_assert(nf_instance->id);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_PATCH;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_NF_INSTANCES;
    message.h.resource.component[1] = nf_instance->id;

    message.http.content_type = (char *)OGS_SBI_CONTENT_PATCH_TYPE;

    PatchItemList = OpenAPI_list_create();
    ogs_assert(PatchItemList);

    memset(&item, 0, sizeof(item));
    item.op = OpenAPI_patch_operation_replace;
    item.path = (char *)"/nfStatus";
    item.value = OpenAPI_any_type_create_string(
        OpenAPI_nf_status_ToString(OpenAPI_nf_status_REGISTERED));
    ogs_assert(item.value);

    OpenAPI_list_add(PatchItemList, &item);

    message.PatchItemList = PatchItemList;

    request = ogs_sbi_build_request(&message);

    OpenAPI_list_free(PatchItemList);
    OpenAPI_any_type_free(item.value);

    return request;
}

ogs_sbi_request_t *ogs_nnrf_nfm_build_de_register(void)
{
    ogs_sbi_nf_instance_t *nf_instance = NULL;

    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    nf_instance = ogs_sbi_self()->nf_instance;
    ogs_assert(nf_instance);
    ogs_assert(nf_instance->id);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_DELETE;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_NF_INSTANCES;
    message.h.resource.component[1] = nf_instance->id;

    request = ogs_sbi_build_request(&message);

    return request;
}

ogs_sbi_request_t *ogs_nnrf_nfm_build_status_subscribe(
        ogs_sbi_subscription_t *subscription)
{
    ogs_sbi_message_t message;
    ogs_sbi_header_t header;
    ogs_sbi_request_t *request = NULL;
    ogs_sbi_server_t *server = NULL;

    OpenAPI_subscription_data_t *SubscriptionData = NULL;
    OpenAPI_subscription_data_subscr_cond_t SubscrCond;

    ogs_assert(subscription);
    ogs_assert(subscription->req_nf_type);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_POST;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS;

    SubscriptionData = ogs_calloc(1, sizeof(*SubscriptionData));
    ogs_expect_or_return_val(SubscriptionData, NULL);

    server = ogs_list_first(&ogs_sbi_self()->server_list);
    ogs_expect_or_return_val(server, NULL);

    memset(&header, 0, sizeof(header));
    header.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    header.api.version = (char *)OGS_SBI_API_V1;
    header.resource.component[0] =
            (char *)OGS_SBI_RESOURCE_NAME_NF_STATUS_NOTIFY;
    SubscriptionData->nf_status_notification_uri =
                        ogs_sbi_server_uri(server, &header);
    ogs_expect_or_return_val(
            SubscriptionData->nf_status_notification_uri, NULL);

	SubscriptionData->req_nf_type = subscription->req_nf_type;
    SubscriptionData->req_nf_instance_id = subscription->req_nf_instance_id;

    OGS_SBI_FEATURES_SET(subscription->requester_features,
            OGS_SBI_NNRF_NFM_SERVICE_MAP);
    SubscriptionData->requester_features =
        ogs_uint64_to_string(subscription->requester_features);
    ogs_expect_or_return_val(SubscriptionData->requester_features, NULL);

    memset(&SubscrCond, 0, sizeof(SubscrCond));
    if (subscription->subscr_cond.nf_type) {
        SubscrCond.nf_type = subscription->subscr_cond.nf_type;
        SubscriptionData->subscr_cond = &SubscrCond;
    }

    message.SubscriptionData = SubscriptionData;

    request = ogs_sbi_build_request(&message);

    ogs_free(SubscriptionData->nf_status_notification_uri);
    ogs_free(SubscriptionData->requester_features);
    ogs_free(SubscriptionData);

    return request;
}

ogs_sbi_request_t *ogs_nnrf_nfm_build_status_unsubscribe(
        ogs_sbi_subscription_t *subscription)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    ogs_assert(subscription);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_DELETE;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_SUBSCRIPTIONS;
    message.h.resource.component[1] = subscription->id;

    request = ogs_sbi_build_request(&message);

    return request;
}

ogs_sbi_request_t *ogs_nnrf_nfm_build_profile_retrieve(char *nf_instance_id)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    ogs_assert(nf_instance_id);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_GET;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_NFM;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_NF_INSTANCES;
    message.h.resource.component[1] = nf_instance_id;

    request = ogs_sbi_build_request(&message);

    return request;
}

ogs_sbi_request_t *ogs_nnrf_disc_build_discover(
        ogs_sbi_service_type_e service_type,
        ogs_sbi_discovery_option_t *discovery_option)
{
    ogs_sbi_message_t message;
    ogs_sbi_request_t *request = NULL;

    OpenAPI_nf_type_e target_nf_type = OpenAPI_nf_type_NULL;
    OpenAPI_nf_type_e requester_nf_type = OpenAPI_nf_type_NULL;

    ogs_assert(service_type);
    target_nf_type = ogs_sbi_service_type_to_nf_type(service_type);
    ogs_assert(target_nf_type);

    ogs_assert(ogs_sbi_self()->nf_instance);
    requester_nf_type = ogs_sbi_self()->nf_instance->nf_type;
    ogs_assert(requester_nf_type);

    memset(&message, 0, sizeof(message));
    message.h.method = (char *)OGS_SBI_HTTP_METHOD_GET;
    message.h.service.name = (char *)OGS_SBI_SERVICE_NAME_NNRF_DISC;
    message.h.api.version = (char *)OGS_SBI_API_V1;
    message.h.resource.component[0] =
        (char *)OGS_SBI_RESOURCE_NAME_NF_INSTANCES;

    message.param.target_nf_type = target_nf_type;
    message.param.requester_nf_type = requester_nf_type;

    message.param.discovery_option = discovery_option;

    request = ogs_sbi_build_request(&message);

    return request;
}
