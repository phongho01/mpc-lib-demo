#include "cosigner/cmp_offline_refresh_service.h"
#include "cosigner/cmp_signature_preprocessed_data.h"
#include "utils.h"
#include "cosigner/cosigner_exception.h"
#include "cosigner/cmp_key_persistency.h"
#include "cosigner/platform_service.h"
#include "cosigner/prf.h"
#include "crypto/elliptic_curve_algebra/elliptic_curve256_algebra.h"
#include "logging/logging_t.h"

namespace fireblocks
{
namespace common
{
namespace cosigner
{

const std::unique_ptr<elliptic_curve256_algebra_ctx_t, void(*)(elliptic_curve256_algebra_ctx_t*)> cmp_offline_refresh_service::_secp256k1(elliptic_curve256_new_secp256k1_algebra(), elliptic_curve256_algebra_ctx_free);
const std::unique_ptr<elliptic_curve256_algebra_ctx_t, void(*)(elliptic_curve256_algebra_ctx_t*)> cmp_offline_refresh_service::_secp256r1(elliptic_curve256_new_secp256r1_algebra(), elliptic_curve256_algebra_ctx_free);
const std::unique_ptr<elliptic_curve256_algebra_ctx_t, void(*)(elliptic_curve256_algebra_ctx_t*)> cmp_offline_refresh_service::_ed25519(elliptic_curve256_new_ed25519_algebra(), elliptic_curve256_algebra_ctx_free);
const std::unique_ptr<elliptic_curve256_algebra_ctx_t, void(*)(elliptic_curve256_algebra_ctx_t*)> cmp_offline_refresh_service::_stark(elliptic_curve256_new_stark_algebra(), elliptic_curve256_algebra_ctx_free);

void cmp_offline_refresh_service::refresh_key_request(const std::string& tenant_id, const std::string& key_id, const std::string& request_id, const std::set<uint64_t>& players_ids, std::map<uint64_t, byte_vector_t>& encrypted_seeds)
{
    if (tenant_id.compare(_key_persistency.get_tenantid_from_keyid(key_id)) != 0)
    {
        LOG_ERROR("key id %s is not part of tenant %s", key_id.c_str(), tenant_id.c_str());
        throw cosigner_exception(cosigner_exception::UNAUTHORIZED);
    }

    cmp_key_metadata metadata;
    _key_persistency.load_key_metadata(key_id, metadata, false);

    if (!metadata.ttl)
    {
        // LOG_WARN("Got key refresh request for key %s but this key has no ttl, this is strange", key_id.c_str());
    }

    if (players_ids.size() > metadata.n)
    {
        LOG_ERROR("got different number of players for keyid = %s, the key has %u players but the request has %lu players", key_id.c_str(), metadata.n, players_ids.size());
        throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
    }

    for (auto i = players_ids.begin(); i != players_ids.end(); ++i)
    {
        if (metadata.players_info.find(*i) == metadata.players_info.end())
        {
            LOG_ERROR("playerid %lu is not part of key, for keyid = %s", *i, key_id.c_str());
            throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
        }
    }

    uint64_t my_id = _service.get_id_from_keyid(key_id);
    std::map<uint64_t, byte_vector_t> player_id_to_seed;
    for (auto i = players_ids.begin(); i != players_ids.end(); ++i)
    {
        const uint64_t player_id = *i;
        if (player_id == my_id)
            continue;

        commitments_sha256_t seed;
        _service.gen_random(sizeof(commitments_sha256_t), seed);
        byte_vector_t seed_vec(seed, &seed[sizeof(commitments_sha256_t)]);
        player_id_to_seed[player_id] = seed_vec;
        encrypted_seeds[player_id] = _service.encrypt_for_player(player_id, seed_vec);
    }
    _refresh_key_persistency.store_refresh_key_seeds(request_id, player_id_to_seed);
}

static void validate_prfs_sizes(const std::vector<prf>& mine, const std::vector<prf>& other, const std::string& prfs_aad)
{
    if (mine.size() != other.size())
    {
        LOG_ERROR("The number %lu of %s prfs generated by seeds sent by the cosigner isn't equal to the number %lu of prfs generated by seeds received from other cosigners", mine.size(), prfs_aad.c_str(), other.size());
        throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
    }
}

void cmp_offline_refresh_service::refresh_key(const std::string& key_id, const std::string& request_id, const std::map<uint64_t, std::map<uint64_t, byte_vector_t>>& encrypted_seeds, std::string& public_key)
{
    verify_tenant_id(_service, _key_persistency, key_id);
    cmp_key_metadata metadata;
    _key_persistency.load_key_metadata(key_id, metadata, false);

    if (encrypted_seeds.size() > metadata.n)
    {
        LOG_ERROR("got different number of players for keyid = %s, the key has %u players but the request has %lu players", key_id.c_str(), metadata.n, encrypted_seeds.size());
        throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
    }

    for (auto i = encrypted_seeds.begin(); i != encrypted_seeds.end(); ++i)
    {
        if (metadata.players_info.find(i->first) == metadata.players_info.end())
        {
            LOG_ERROR("playerid %lu is not part of key, for keyid = %s", i->first, key_id.c_str());
            throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
        }
    }

    std::vector<prf> mine_prfs_x, mine_prfs_k, mine_prfs_chi;
    std::vector<prf> other_prfs_x, other_prfs_k, other_prfs_chi;
    const std::string ref_x = (request_id + "x"), ref_k = (request_id + "k"), ref_chi = (request_id + "chi");

    uint64_t my_id = _service.get_id_from_keyid(key_id);
    std::map<uint64_t, byte_vector_t> player_id_to_seed;
    _refresh_key_persistency.load_refresh_key_seeds(request_id, player_id_to_seed);
    for (auto i = encrypted_seeds.begin(); i != encrypted_seeds.end(); ++i)
    {
        const uint64_t player_id = i->first;
        if (player_id == my_id)
            continue;
        auto my_seed_it = i->second.find(my_id);
        if (my_seed_it == i->second.end())
        {
            LOG_ERROR("Player %lu didn't send seed to me", player_id);
            throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
        }

        auto decrypt_seed = _service.decrypt_message(my_seed_it->second);
        if (decrypt_seed.size() != sizeof(commitments_sha256_t))
        {
            LOG_ERROR("Player %lu sent invalid seed to me", player_id);
            throw cosigner_exception(cosigner_exception::INVALID_PARAMETERS);
        }
        commitments_sha256_t seed_from_player;
        memcpy(seed_from_player, decrypt_seed.data(), decrypt_seed.size());
        other_prfs_x.push_back(prf(seed_from_player, ref_x));
        other_prfs_k.push_back(prf(seed_from_player, ref_k));
        other_prfs_chi.push_back(prf(seed_from_player, ref_chi));

        commitments_sha256_t seed_sent_to_player;
        memcpy(seed_sent_to_player, player_id_to_seed.at(player_id).data(), sizeof(commitments_sha256_t));
        mine_prfs_x.push_back(prf(seed_sent_to_player, ref_x));
        mine_prfs_k.push_back(prf(seed_sent_to_player, ref_k));
        mine_prfs_chi.push_back(prf(seed_sent_to_player, ref_chi));
    }

    validate_prfs_sizes(mine_prfs_x, other_prfs_x, ref_x);
    validate_prfs_sizes(mine_prfs_k, other_prfs_k, ref_k);
    validate_prfs_sizes(mine_prfs_chi, other_prfs_chi, ref_chi);

    LOG_INFO("Refreshing key %s", key_id.c_str());
    cosigner_sign_algorithm algo;
    elliptic_curve_scalar private_key;
    _key_persistency.load_key(key_id, algo, private_key.data);
    auto algebra = get_algebra(metadata.algorithm);
    elliptic_curve_scalar new_private_key;
    memcpy(new_private_key.data, private_key.data, sizeof(elliptic_curve256_scalar_t));

    for (auto i = 0ul; i != mine_prfs_x.size(); ++i)
    {
        commitments_sha256_t val;
        other_prfs_x[i].run(0, val);
        throw_cosigner_exception(algebra->add_scalars(algebra, &new_private_key.data, new_private_key.data, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
        mine_prfs_x[i].run(0, val);
        throw_cosigner_exception(algebra->sub_scalars(algebra, &new_private_key.data, new_private_key.data, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
    }

    LOG_INFO("Refreshing presigning data for key %s", key_id.c_str());
    _refresh_key_persistency.transform_preprocessed_data_and_store_temporary(key_id, request_id, [algebra, &private_key, &new_private_key, &mine_prfs_k, &mine_prfs_chi, &other_prfs_k, &other_prfs_chi] (uint64_t index, cmp_signature_preprocessed_data& data)
    {
        elliptic_curve256_scalar_t k;
        memcpy(k, data.k.data, sizeof(elliptic_curve256_scalar_t));
        elliptic_curve256_scalar_t chi;
        memcpy(chi, data.chi.data, sizeof(elliptic_curve256_scalar_t));
        commitments_sha256_t val;

        for (auto i = 0ul; i != mine_prfs_k.size(); ++i)
        {
            other_prfs_k[i].run(index, val);
            throw_cosigner_exception(algebra->add_scalars(algebra, &k, k, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
            mine_prfs_k[i].run(index, val);
            throw_cosigner_exception(algebra->sub_scalars(algebra, &k, k, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
        }

        elliptic_curve256_scalar_t tmp;
        throw_cosigner_exception(algebra->mul_scalars(algebra, &tmp, data.k.data, sizeof(elliptic_curve256_scalar_t), reinterpret_cast<const uint8_t*>(private_key.data), sizeof(elliptic_curve256_scalar_t)));
        throw_cosigner_exception(algebra->add_scalars(algebra, &chi, chi, sizeof(elliptic_curve256_scalar_t), tmp, sizeof(elliptic_curve256_scalar_t)));
        throw_cosigner_exception(algebra->mul_scalars(algebra, &tmp, k, sizeof(elliptic_curve256_scalar_t), new_private_key.data, sizeof(elliptic_curve256_scalar_t)));
        throw_cosigner_exception(algebra->sub_scalars(algebra, &chi, chi, sizeof(elliptic_curve256_scalar_t), tmp, sizeof(elliptic_curve256_scalar_t)));

        for (auto i = 0ul; i != mine_prfs_chi.size(); ++i)
        {
            other_prfs_chi[i].run(index, val);
            throw_cosigner_exception(algebra->add_scalars(algebra, &chi, chi, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
            mine_prfs_chi[i].run(index, val);
            throw_cosigner_exception(algebra->sub_scalars(algebra, &chi, chi, sizeof(elliptic_curve256_scalar_t), val, sizeof(commitments_sha256_t)));
        }

        memcpy(data.chi.data, chi, sizeof(elliptic_curve256_scalar_t));
        memcpy(data.k.data, k, sizeof(elliptic_curve256_scalar_t));
    });
    _refresh_key_persistency.delete_refresh_key_seeds(request_id);

    LOG_INFO("Storing new temporary key %s (request_id), key_id: %s", request_id.c_str(), key_id.c_str());
    _refresh_key_persistency.store_temporary_key(request_id, algo, new_private_key);
    public_key.assign((const char*)metadata.public_key, algebra->point_size(algebra));
}

void cmp_offline_refresh_service::refresh_key_fast_ack(const std::string& tenant_id, const std::string& key_id, const std::string& request_id)
{
    if (tenant_id.compare(_key_persistency.get_tenantid_from_keyid(key_id)) != 0)
    {
        LOG_ERROR("key id %s is not part of tenant %s", key_id.c_str(), tenant_id.c_str());
        throw cosigner_exception(cosigner_exception::UNAUTHORIZED);
    }
    _refresh_key_persistency.commit(key_id, request_id);

    // LOG_INFO("backuping keyid %s..", key_id.c_str());
    cmp_key_metadata metadata;
    _key_persistency.load_key_metadata(key_id, metadata, false);
    elliptic_curve_scalar key;
    cosigner_sign_algorithm algo;
    _key_persistency.load_key(key_id, algo, key.data);

    auxiliary_keys aux;
    _key_persistency.load_auxiliary_keys(key_id, aux);
    if (!_service.backup_key(key_id, metadata.algorithm, key.data, metadata, aux))
    {
        LOG_WARN("failed to backup key id %s, but refresh key succeeded", key_id.c_str());
        throw cosigner_exception(cosigner_exception::BACKUP_FAILED);
    }
}

void cmp_offline_refresh_service::cancel_refresh_key(const std::string& request_id)
{
    try
    {
        _refresh_key_persistency.delete_refresh_key_seeds(request_id);
        _refresh_key_persistency.delete_temporary_key(request_id);
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Exception %s in deleting temp request %s data, ignoring", e.what(), request_id.c_str());
    }
}

elliptic_curve256_algebra_ctx_t* cmp_offline_refresh_service::get_algebra(cosigner_sign_algorithm algorithm)
{
    switch (algorithm)
    {
    case ECDSA_SECP256K1: return _secp256k1.get();
    case ECDSA_SECP256R1: return _secp256r1.get();
    case EDDSA_ED25519: return _ed25519.get();
    case ECDSA_STARK: return _stark.get();
    default:
        throw cosigner_exception(cosigner_exception::UNKNOWN_ALGORITHM);
    }
}

}
}
}