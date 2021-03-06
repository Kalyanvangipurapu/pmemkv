/*
 * Copyright 2017-2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vcmap.h"

#include <iostream>

#define DO_LOG 0
#define LOG(msg) if (DO_LOG) std::cout << "[vcmap] " << msg << "\n"

namespace pmemkv {
namespace vcmap {

VCMap::VCMap(const string& path, size_t size) : kv_allocator(path, size), ch_allocator(kv_allocator),
             pmem_kv_container(std::scoped_allocator_adaptor<kv_allocator_t>(kv_allocator)) {
    LOG("Started ok");
}

VCMap::~VCMap() {
    LOG("Stopped ok");
}

void VCMap::All(void* context, KVAllCallback* callback) {
    LOG("All");
    for (auto& iterator : pmem_kv_container) {
        (*callback)(context, (int32_t) iterator.first.size(), iterator.first.c_str());
    }
}

int64_t VCMap::Count() {
    LOG("Count");
    return pmem_kv_container.size();
}

void VCMap::Each(void* context, KVEachCallback* callback) {
    LOG("Each");
    for (auto& iterator : pmem_kv_container) {
        (*callback)(context, (int32_t) iterator.first.size(), iterator.first.c_str(),
                (int32_t) iterator.second.size(), iterator.second.c_str());
    }
}

KVStatus VCMap::Exists(const string& key) {
    LOG("Exists for key=" << key);
    map_t::const_accessor result;
    const bool result_found = pmem_kv_container.find(result, pmem_string(key.c_str(), key.size(), ch_allocator));
    return (result_found ? OK : NOT_FOUND);
}

void VCMap::Get(void* context, const string& key, KVGetCallback* callback) {
    LOG("Get key=" << key);
    map_t::const_accessor result;
    const bool result_found = pmem_kv_container.find(result, pmem_string(key.c_str(), key.size(), ch_allocator));
    if (!result_found) {
        LOG("  key not found");
        return;
    }
    (*callback)(context, (int32_t) result->second.size(), result->second.c_str());
}

KVStatus VCMap::Put(const string& key, const string& value) {
    LOG("Put key=" << key << ", value.size=" << to_string(value.size()));
    try {
        map_t::value_type kv_pair{pmem_string(key.c_str(), key.size(), ch_allocator),
                                  pmem_string(value.c_str(), value.size(), ch_allocator)};
        bool result = pmem_kv_container.insert(kv_pair);
        if (!result) {
            map_t::accessor result_found;
            pmem_kv_container.find(result_found, kv_pair.first);
            result_found->second = kv_pair.second;
        }
        return OK;
    } catch (std::bad_alloc e) {
        LOG("Put failed due to exception, " << e.what());
        return FAILED;
    } catch (pmem::transaction_alloc_error e) {
        LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
        return FAILED;
    } catch (pmem::transaction_error e) {
        LOG("Put failed due to pmem::transaction_error, " << e.what());
        return FAILED;
    }
}

KVStatus VCMap::Remove(const string& key) {
    LOG("Remove key=" << key);
    try {
        size_t erased = pmem_kv_container.erase(pmem_string(key.c_str(), key.size(), ch_allocator));
        return (erased == 1) ? OK : NOT_FOUND;
    } catch (std::bad_alloc e) {
        LOG("Put failed due to exception, " << e.what());
        return FAILED;
    } catch (pmem::transaction_alloc_error e) {
        LOG("Put failed due to pmem::transaction_alloc_error, " << e.what());
        return FAILED;
    } catch (pmem::transaction_error e) {
        LOG("Put failed due to pmem::transaction_error, " << e.what());
        return FAILED;
    }
}

} // namespace vcmap
} // namespace pmemkv
