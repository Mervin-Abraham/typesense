#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <art.h>
#include <number.h>
#include <sparsepp.h>
#include <store.h>
#include <topster.h>
#include <json.hpp>
#include <field.h>
#include <option.h>
#include <set>
#include "string_utils.h"
#include "num_tree.h"
#include "magic_enum.hpp"
#include "match_score.h"
#include "posting_list.h"
#include "threadpool.h"
#include "adi_tree.h"
#include "tsl/htrie_set.h"
#include <tsl/htrie_map.h>
#include <or_iterator.h>
#include "id_list.h"
#include "synonym_index.h"
#include "override.h"
#include "vector_query_ops.h"
#include "hnswlib/hnswlib.h"
#include "filter.h"
#include "facet_index.h"
#include "numeric_range_trie.h"
#include "geopolygon_index.h"


static constexpr size_t ARRAY_FACET_DIM = 4;
using facet_map_t = spp::sparse_hash_map<uint32_t, facet_hash_values_t>;
using single_val_facet_map_t = spp::sparse_hash_map<uint32_t, uint32_t>;
using array_mapped_facet_t = std::array<facet_map_t*, ARRAY_FACET_DIM>;
using array_mapped_single_val_facet_t = std::array<single_val_facet_map_t*, ARRAY_FACET_DIM>;

static constexpr size_t ARRAY_INFIX_DIM = 4;
using array_mapped_infix_t = std::vector<tsl::htrie_set<char>*>;

struct token_t {
    size_t position;
    std::string value;

    bool is_prefix_searched;
    uint32_t root_len;        // if prefix searched, length of the root token
    uint32_t num_typos;

    token_t() {};

    token_t(size_t position, const std::string& value, bool is_prefix_searched, uint32_t root_len, uint32_t num_typos):
            position(position), value(value), is_prefix_searched(is_prefix_searched),
            root_len(root_len), num_typos(num_typos) {

    }
};

// FIXME: deprecated
struct token_candidates {
    token_t token;
    size_t cost;
    bool prefix_search;
    std::vector<art_leaf*> candidates;
};

struct tok_candidates {
    token_t token;
    size_t cost;
    bool prefix_search;
    std::vector<std::string> candidates;
};

struct query_tokens_t {
    std::vector<token_t> q_include_tokens;
    std::vector<token_t> q_unstemmed_tokens;
    std::vector<std::vector<std::string>> q_exclude_tokens;
    std::vector<std::vector<std::string>> q_phrases;
    std::vector<std::vector<std::string>> q_synonyms;
};

enum enable_t {
    always,
    fallback,
    off
};

struct search_field_t {
    std::string name;
    std::string str_name;   // for lookup of non-string fields in art index
    size_t weight;
    size_t num_typos;
    bool prefix;
    enable_t infix;

    search_field_t(const std::string& name, const std::string& str_name, size_t weight, size_t num_typos,
                   bool prefix, enable_t infix):
            name(name), str_name(str_name), weight(weight), num_typos(num_typos), prefix(prefix), infix(infix) { }
};

enum text_match_type_t {
    max_score,
    sum_score,
    max_weight
};

enum drop_tokens_mode_t {
    left_to_right,
    right_to_left,
    both_sides,
};

struct drop_tokens_param_t {
    drop_tokens_mode_t mode = right_to_left;
    size_t token_limit = 1000;

    drop_tokens_param_t() {

    }

    drop_tokens_param_t(drop_tokens_mode_t mode, size_t token_limit) : mode(mode), token_limit(token_limit) {}
};

enum facet_index_type_t {
    exhaustive,
    top_values,
    automatic,
};

struct search_args {
    std::vector<query_tokens_t> field_query_tokens;
    std::vector<search_field_t> search_fields;
    const text_match_type_t match_type;
    std::vector<facet>& facets;
    std::vector<std::pair<uint32_t, uint32_t>>& included_ids;
    std::vector<uint32_t> excluded_ids;
    std::vector<sort_by> sort_fields_std;
    facet_query_t facet_query;
    std::vector<uint32_t> num_typos;
    size_t max_facet_values;
    size_t fetch_size;
    size_t per_page;
    size_t offset;
    token_ordering token_order;
    std::vector<bool> prefixes;
    size_t drop_tokens_threshold;
    size_t typo_tokens_threshold;
    std::vector<std::string> group_by_fields;
    size_t group_limit;
    bool group_missing_values;
    std::string default_sorting_field;
    bool prioritize_exact_match;
    bool prioritize_token_position;
    bool prioritize_num_matching_fields;
    size_t all_result_ids_len;
    bool exhaustive_search;
    size_t concurrency;
    size_t search_cutoff_ms;
    size_t min_len_1typo;
    size_t min_len_2typo;
    size_t max_candidates;
    std::vector<enable_t> infixes;
    const size_t max_extra_prefix;
    const size_t max_extra_suffix;
    const size_t facet_query_num_typos;
    const bool filter_curated_hits;
    const enable_t split_join_tokens;
    tsl::htrie_map<char, token_leaf> qtoken_set;

    spp::sparse_hash_map<uint64_t, uint32_t> groups_processed;
    std::vector<std::vector<art_leaf*>> searched_queries;
    Topster<KV>* topster = nullptr;
    Topster<KV>* curated_topster = nullptr;
    std::vector<std::vector<KV*>> raw_result_kvs;
    std::vector<std::vector<KV*>> override_result_kvs;

    vector_query_t& vector_query;
    size_t facet_sample_percent;
    size_t facet_sample_threshold;
    drop_tokens_param_t drop_tokens_mode;

    std::unique_ptr<filter_node_t> filter_tree_root_guard;
    bool enable_lazy_filter;
    size_t max_filter_by_candidates;

    std::vector<facet_index_type_t> facet_index_types;
    bool enable_typos_for_numerical_tokens;
    bool enable_synonyms;
    bool synonym_prefix;
    uint32_t synonym_num_typos;
    bool enable_typos_for_alpha_numerical_tokens;
    bool rerank_hybrid_matches;
    bool validate_field_names;
    size_t found_count = 0;
    size_t found_docs = 0;

    search_args(std::vector<query_tokens_t> field_query_tokens, std::vector<search_field_t> search_fields,
                const text_match_type_t match_type, std::vector<facet>& facets,
                std::vector<std::pair<uint32_t, uint32_t>>& included_ids, std::vector<uint32_t> excluded_ids,
                std::vector<sort_by>& sort_fields_std, facet_query_t facet_query, const std::vector<uint32_t>& num_typos,
                size_t max_facet_values, size_t fetch_size, size_t per_page, size_t offset, token_ordering token_order,
                const std::vector<bool>& prefixes, size_t drop_tokens_threshold, size_t typo_tokens_threshold,
                const std::vector<std::string>& group_by_fields, size_t group_limit,
                const bool group_missing_values,
                const string& default_sorting_field, bool prioritize_exact_match,
                const bool prioritize_token_position, const bool prioritize_num_matching_fields, bool exhaustive_search,
                size_t concurrency, size_t search_cutoff_ms,
                size_t min_len_1typo, size_t min_len_2typo, size_t max_candidates, const std::vector<enable_t>& infixes,
                const size_t max_extra_prefix, const size_t max_extra_suffix, const size_t facet_query_num_typos,
                const bool filter_curated_hits, const enable_t split_join_tokens, vector_query_t& vector_query,
                size_t facet_sample_percent, size_t facet_sample_threshold, drop_tokens_param_t drop_tokens_mode,
                std::unique_ptr<filter_node_t>&& filter_tree_root_guard, bool enable_lazy_filter, const size_t max_filter_by_candidates,
                std::vector<facet_index_type_t>& facet_index_types, bool enable_typos_for_numerical_tokens,
                bool enable_synonyms, bool synonym_prefix, uint32_t synonym_num_typos,
                bool enable_typos_for_alpha_numerical_tokens, bool rerank_hybrid_matches, const bool& validate_field_names) :
            field_query_tokens(field_query_tokens),
            search_fields(search_fields), match_type(match_type), facets(facets),
            included_ids(included_ids), excluded_ids(excluded_ids), sort_fields_std(std::move(sort_fields_std)),
            facet_query(facet_query), num_typos(num_typos), max_facet_values(max_facet_values),
            fetch_size(fetch_size), per_page(per_page),
            offset(offset), token_order(token_order), prefixes(prefixes),
            drop_tokens_threshold(drop_tokens_threshold), typo_tokens_threshold(typo_tokens_threshold),
            group_by_fields(group_by_fields), group_limit(group_limit),
            group_missing_values(group_missing_values),
            default_sorting_field(default_sorting_field),
            prioritize_exact_match(prioritize_exact_match), prioritize_token_position(prioritize_token_position),
            prioritize_num_matching_fields(prioritize_num_matching_fields),
            all_result_ids_len(0), exhaustive_search(exhaustive_search), concurrency(concurrency),
            search_cutoff_ms(search_cutoff_ms),
            min_len_1typo(min_len_1typo), min_len_2typo(min_len_2typo), max_candidates(max_candidates),
            infixes(infixes), max_extra_prefix(max_extra_prefix), max_extra_suffix(max_extra_suffix),
            facet_query_num_typos(facet_query_num_typos), filter_curated_hits(filter_curated_hits),
            split_join_tokens(split_join_tokens), vector_query(vector_query),
            facet_sample_percent(facet_sample_percent), facet_sample_threshold(facet_sample_threshold),
            drop_tokens_mode(drop_tokens_mode), filter_tree_root_guard(std::move(filter_tree_root_guard)),
            enable_lazy_filter(enable_lazy_filter), max_filter_by_candidates(max_filter_by_candidates),
            facet_index_types(std::move(facet_index_types)),
            enable_typos_for_numerical_tokens(enable_typos_for_numerical_tokens), enable_synonyms(enable_synonyms),
            synonym_prefix(synonym_prefix), synonym_num_typos(synonym_num_typos),
            enable_typos_for_alpha_numerical_tokens(enable_typos_for_alpha_numerical_tokens),
            rerank_hybrid_matches(rerank_hybrid_matches), validate_field_names(validate_field_names) {

    }

    ~search_args() {
        delete topster;
        delete curated_topster;

        for (auto& sort_by_clause: sort_fields_std) {
            for (auto& eval_ids: sort_by_clause.eval.eval_ids_vec) {
                delete [] eval_ids;
            }

            for (uint32_t i = 0; i < sort_by_clause.eval_expressions.size(); i++) {
                delete sort_by_clause.eval.filter_trees[i];
            }

            delete [] sort_by_clause.eval.filter_trees;
        }
    };
};

struct offsets_facet_hashes_t {
    // token to offsets
    std::unordered_map<std::string, std::vector<uint32_t>> offsets;
};

struct index_record {
    size_t position;                    // position of record in the original request
    uint32_t seq_id;

    nlohmann::json doc;                 // actual document sent in request (could be partial)
    nlohmann::json old_doc;             // previously stored *full* document from disk
    nlohmann::json new_doc;             // new *full* document to be stored into disk
    nlohmann::json del_doc;             // document containing the fields that should be deleted

    nlohmann::json embedding_res;       // embedding result
    int embedding_status_code;          // embedding status code

    index_operation_t operation;
    bool is_update;

    // pre-processed data primed for indexing
    std::unordered_map<std::string, offsets_facet_hashes_t> field_index;
    int64_t points;

    Option<bool> indexed;               // indicates if the indexing operation was a success

    DIRTY_VALUES dirty_values;

    index_record(size_t record_pos, uint32_t seq_id, const nlohmann::json& doc, index_operation_t operation,
                 const DIRTY_VALUES& dirty_values):
            position(record_pos), seq_id(seq_id), doc(doc), operation(operation), is_update(false),
            indexed(false), dirty_values(dirty_values) {

    }

    index_record(index_record&& rhs) = default;

    index_record& operator=(index_record&& mE) = default;

    void index_failure(const uint32_t err_code, const std::string & err_msg) {
        indexed = Option<bool>(err_code, err_msg);
    }

    void index_success() {
        indexed = Option<bool>(true);
    }
};

class VectorFilterFunctor: public hnswlib::BaseFilterFunctor {
    filter_result_iterator_t* const filter_result_iterator;

    const uint32_t* excluded_ids = nullptr;
    const uint32_t excluded_ids_length = 0;

public:

    explicit VectorFilterFunctor(filter_result_iterator_t* const filter_result_iterator,
                                 const uint32_t* excluded_ids = nullptr, const uint32_t excluded_ids_length = 0) :
                                filter_result_iterator(filter_result_iterator),
                                excluded_ids(excluded_ids), excluded_ids_length(excluded_ids_length) {}

    bool operator()(hnswlib::labeltype id) override {
        if (filter_result_iterator->approx_filter_ids_length == 0 && excluded_ids_length == 0) {
            return true;
        }

        if(excluded_ids_length > 0 && excluded_ids && std::binary_search(excluded_ids, excluded_ids + excluded_ids_length, id)) {
            return false;
        }

        if(filter_result_iterator->approx_filter_ids_length == 0) {
            return true;
        }

        filter_result_iterator->reset();
        return filter_result_iterator->is_valid(id) == 1;
    }
};

struct hnsw_index_t {
    hnswlib::InnerProductSpace* space;
    hnswlib::HierarchicalNSW<float>* vecdex;
    size_t num_dim;
    vector_distance_type_t distance_type;

    // ensures that this index is not dropped when it's being repaired
    std::mutex repair_m;

    hnsw_index_t(size_t num_dim, size_t init_size, vector_distance_type_t distance_type, size_t M = 16, size_t ef_construction = 200) :
        space(new hnswlib::InnerProductSpace(num_dim)),
        vecdex(new hnswlib::HierarchicalNSW<float>(space, init_size, M, ef_construction, 100, true)),
        num_dim(num_dim), distance_type(distance_type) {

    }

    ~hnsw_index_t() {
        std::lock_guard lk(repair_m);
        delete vecdex;
        delete space;
    }

    // needed for cosine similarity
    static void normalize_vector(const std::vector<float>& src, std::vector<float>& norm_dest) {
        float norm = 0.0f;
        for (float i : src) {
            norm += i * i;
        }
        norm = 1.0f / (sqrtf(norm) + 1e-30f);
        for (size_t i = 0; i < src.size(); i++) {
            norm_dest[i] = src[i] * norm;
        }
    }
};

struct group_by_field_it_t {
    std::string field_name;
    posting_list_t::iterator_t it;
    bool is_array;
};

struct Hasher32 {
    // Helps to spread the hash key and is used for sort index.
    // see: https://github.com/greg7mdp/sparsepp/issues/21#issuecomment-270816275
    size_t operator()(uint32_t k) const { return (k ^ 2166136261U)  * 16777619UL; }
};

struct negate_left_join_t {
    bool is_negate_join = false;
    size_t excluded_ids_size = 0;
    std::unique_ptr<uint32_t []> excluded_ids = nullptr;

    negate_left_join_t() = default;
};

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

class Index {
private:
    mutable std::shared_mutex mutex;

    std::string name;

    const uint32_t collection_id;

    const Store* store;

    const SynonymIndex* synonym_index;

    ThreadPool* thread_pool;

    size_t num_documents;

    tsl::htrie_map<char, field> search_schema;

    spp::sparse_hash_map<std::string, art_tree*> search_index;

    spp::sparse_hash_map<std::string, num_tree_t*> numerical_index;

    // reference_helper_field => (seq_id => ref_seq_ids)
    // Only used when the reference field is an array type otherwise sort_index is used.
    spp::sparse_hash_map<std::string, num_tree_t*> reference_index;

    /// field_name => ((doc_id, object_index) => ref_doc_id)
    /// Used when a field inside an object array has reference.
    spp::sparse_hash_map<std::string, spp::sparse_hash_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash>*> object_array_reference_index;

    spp::sparse_hash_map<std::string, NumericTrie*> range_index;

    spp::sparse_hash_map<std::string, NumericTrie*> geo_range_index;

    spp::sparse_hash_map<std::string, GeoPolygonIndex*> field_geopolygon_index;

    // geo_array_field => (seq_id => values) used for exact filtering of geo array records
    spp::sparse_hash_map<std::string, spp::sparse_hash_map<uint32_t, int64_t*>*> geo_array_index;

    facet_index_t* facet_index_v4 = nullptr;
  
    // sort_field => (seq_id => value)
    spp::sparse_hash_map<std::string, spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*> sort_index;
    typedef spp::sparse_hash_map<std::string, 
        spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*>::iterator sort_index_iterator;

    // str_sort_field => adi_tree_t
    spp::sparse_hash_map<std::string, adi_tree_t*> str_sort_index;

    // infix field => value
    spp::sparse_hash_map<std::string, array_mapped_infix_t> infix_index;

    // vector field => vector index
    spp::sparse_hash_map<std::string, hnsw_index_t*> vector_index;

    // this is used for wildcard queries
    id_list_t* seq_ids;

    std::vector<char> symbols_to_index;

    std::vector<char> token_separators;

    StringUtils string_utils;

    // used as sentinels

    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> text_match_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> seq_id_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> group_found_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> eval_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> geo_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> str_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> vector_distance_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> vector_query_sentinel_value;
    static spp::sparse_hash_map<uint32_t, int64_t, Hasher32> union_search_index_sentinel_value;

    // Internal utility functions

    static inline uint32_t next_suggestion2(const std::vector<tok_candidates>& token_candidates_vec,
                                            long long int n,
                                            std::vector<token_t>& query_suggestion,
                                            uint64& qhash);

    static inline uint32_t next_suggestion(const std::vector<token_candidates> &token_candidates_vec,
                                       long long int n,
                                       std::vector<art_leaf *>& actual_query_suggestion,
                                       std::vector<art_leaf *>& query_suggestion,
                                       int syn_orig_num_tokens,
                                       uint32_t& token_bits,
                                       uint64& qhash);

    void log_leaves(int cost, const std::string &token, const std::vector<art_leaf *> &leaves) const;

    void do_facets(std::vector<facet> & facets, facet_query_t & facet_query,
                   bool estimate_facets, size_t facet_sample_percent,
                   const std::vector<facet_info_t>& facet_infos,
                   size_t group_limit, const std::vector<std::string>& group_by_fields,
                   const bool group_missing_values,
                   const uint32_t* result_ids, size_t results_size,
                   int max_facet_count, bool is_wildcard_query,
                   const std::vector<facet_index_type_t>& facet_index_types,
                   bool is_group_by_first_pass,
                   std::set<uint32_t>& group_by_missing_value_ids) const;

    bool static_filter_query_eval(const override_t* override, std::vector<std::string>& tokens,
                                  std::unique_ptr<filter_node_t>& filter_tree_root, const bool& validate_field_names) const;

    bool resolve_override(const std::vector<std::string>& rule_tokens, bool exact_rule_match,
                          const std::vector<std::string>& query_tokens,
                          token_ordering token_order, std::set<std::string>& absorbed_tokens,
                          std::string& filter_by_clause,
                          bool enable_typos_for_numerical_tokens,
                          bool enable_typos_for_alpha_numerical_tokens) const;

    bool check_for_overrides(const token_ordering& token_order, const string& field_name, bool slide_window,
                             bool exact_rule_match, std::vector<std::string>& tokens,
                             std::set<std::string>& absorbed_tokens,
                             std::vector<std::string>& field_absorbed_tokens,
                             bool enable_typos_for_numerical_tokens,
                             bool enable_typos_for_alpha_numerical_tokens) const;

    static void aggregate_topster(Topster<KV>* agg_topster, Topster<KV>* index_topster, const bool& is_group_by_first_pass);

    Option<bool> search_all_candidates(const size_t num_search_fields,
                                       const text_match_type_t match_type,
                                       const std::vector<search_field_t>& the_fields,
                                       filter_result_iterator_t* const filter_result_iterator,
                                       const uint32_t* excluded_result_ids, size_t excluded_result_ids_size,
                                       const std::vector<sort_by>& sort_fields,
                                       std::vector<tok_candidates>& token_candidates_vec,
                                       std::vector<std::vector<art_leaf*>>& searched_queries,
                                       tsl::htrie_map<char, token_leaf>& qtoken_set,
                                       const std::vector<token_t>& dropped_tokens,
                                       Topster<KV>*& topster,
                                       spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                       size_t& num_keyword_matches,
                                       uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                       const size_t typo_tokens_threshold,
                                       const size_t group_limit,
                                       const std::vector<std::string>& group_by_fields,
                                       const bool group_missing_values,
                                       const std::vector<token_t>& query_tokens,
                                       const std::vector<uint32_t>& num_typos,
                                       const std::vector<bool>& prefixes,
                                       bool prioritize_exact_match,
                                       const bool prioritize_token_position,
                                       const bool exhaustive_search,
                                       const bool prioritize_num_matching_fields,
                                       const size_t max_candidates,
                                       int syn_orig_num_tokens,
                                       const int* sort_order,
                                       std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                       const std::vector<size_t>& geopoint_indices,
                                       std::set<uint64>& query_hashes,
                                       std::vector<uint32_t>& id_buff,
                                       bool is_group_by_first_pass,
                                       std::set<uint32_t>& group_by_missing_value_ids) const;

    static void popular_fields_of_token(const spp::sparse_hash_map<std::string, art_tree*>& search_index,
                                        const std::string& previous_token,
                                        const std::vector<search_field_t>& the_fields,
                                        const size_t num_search_fields,
                                        std::vector<size_t>& popular_field_ids);

    bool field_is_indexed(const std::string& field_name) const;

    static void tokenize_string(const std::string& text,
                                const field& a_field,
                                const std::vector<char>& symbols_to_index,
                                const std::vector<char>& token_separators,
                                std::unordered_map<std::string, std::vector<uint32_t>>& token_to_offsets);

    static void tokenize_string_array(const std::vector<std::string>& strings,
                                      const field& a_field,
                                      const std::vector<char>& symbols_to_index,
                                      const std::vector<char>& token_separators,
                                      std::unordered_map<std::string, std::vector<uint32_t>>& token_to_offsets);

    void collate_included_ids(const std::vector<token_t>& q_included_tokens,
                              const std::map<size_t, std::map<size_t, uint32_t>> & included_ids_map,
                              Topster<KV>*& curated_topster,
                              const size_t group_limit,
                              const std::vector<std::string>& group_by_fields,
                              const bool group_missing_values,
                              std::vector<std::vector<art_leaf*>> & searched_queries,
                              bool is_group_by_first_pass,
                              std::set<uint32_t>& group_by_missing_value_ids) const;

    static void compute_facet_stats(facet &a_facet, const std::string& raw_value,
                                    const std::string & field_type, const size_t count);

    static void compute_facet_stats(facet &a_facet, const int64_t raw_value, const std::string & field_type);

    static void handle_doc_ops(const tsl::htrie_map<char, field>& search_schema,
                               nlohmann::json& update_doc, const nlohmann::json& old_doc);

    static void get_doc_changes(const index_operation_t op, const tsl::htrie_map<char, field>& embedding_fields,
                                nlohmann::json &update_doc, const nlohmann::json &old_doc, nlohmann::json &new_doc,
                                nlohmann::json &del_doc);

    bool common_results_exist(std::vector<art_leaf*>& leaves, bool must_match_phrase) const;

    static void remove_facet_token(const field& search_field, spp::sparse_hash_map<std::string, art_tree*>& search_index,
                                   const std::string& token, uint32_t seq_id);

    void initialize_facet_indexes(const field& facet_field);

    std::vector<group_by_field_it_t> get_group_by_field_iterators(const std::vector<std::string>&, bool is_reverse=false) const;

    static void batch_embed_fields(std::vector<index_record*>& documents,
                                   const tsl::htrie_map<char, field>& embedding_fields,
                                   const tsl::htrie_map<char, field> & search_schema, const size_t remote_embedding_batch_size = 200,
                                   const size_t remote_embedding_timeout_ms = 60000, const size_t remote_embedding_num_tries = 2);
    
    static void process_embed_results(const std::vector<std::pair<index_record*, std::string>>& values_to_embed_text,
                                      const std::vector<embedding_res_t>& embeddings_text,
                                      const std::vector<std::pair<index_record*, std::string>>& values_to_embed_image,
                                      const std::vector<embedding_res_t>& embeddings_image,
                                      const std::vector<std::pair<index_record*, std::vector<std::string>>>& values_to_embed_personalization,
                                      const std::vector<embedding_res_t>& embeddings_personalization,
                                      const field& the_field);

    void update_async_references(const std::string& collection_name, const field& afield,
                                 std::vector<index_record>& iter_batch,
                                 const std::set<reference_pair_t>& async_referenced_ins = {});

    std::string get_collection_name_with_lock() const {
        std::shared_lock lock(mutex);
        return get_collection_name();
    }

    std::string get_collection_name() const {
        // Index name is: collection_name + std::to_string(0)
        return name.empty() ? "" : name.substr(0, name.size() - 1);
    }

    Option<uint32_t> get_sort_index_value(const std::string& field_name,
                                          const uint32_t& seq_id) const;

    Option<int64_t> get_geo_distance(const std::string& geo_field_name, const uint32_t& seq_id,
                                     const S2LatLng& reference_lat_lng, const bool& round_distance = false) const;

    Option<uint32_t> get_ref_seq_id_helper(const sort_by& sort_field, const uint32_t& seq_id, std::string& prev_coll_name,
                                           std::map<std::string, reference_filter_result_t> const*& references,
                                           std::string& ref_coll_name) const;

public:
    // for limiting number of results on multiple candidates / query rewrites
    enum {TYPO_TOKENS_THRESHOLD = 1};

    // for limiting number of fields that can be searched on
    enum {FIELD_LIMIT_NUM = 100};

    // Values 0 to 15 are allowed
    enum {FIELD_MAX_WEIGHT = 15};

    enum {COMBINATION_MAX_LIMIT = 10000};
    enum {COMBINATION_MIN_LIMIT = 10};

    enum {NUM_CANDIDATES_DEFAULT_MIN = 4};
    enum {NUM_CANDIDATES_DEFAULT_MAX = 10};

    // If the number of results found is less than this threshold, Typesense will attempt to drop the tokens
    // in the query that have the least individual hits one by one until enough results are found.
    static const int DROP_TOKENS_THRESHOLD = 1;

    enum {DEFAULT_TOPSTER_SIZE = 250};

    static const size_t GROUP_LIMIT_MAX = 99;

    /// Value used when async_reference is true and a reference doc is not found.
    static constexpr int64_t reference_helper_sentinel_value = UINT32_MAX;

    Index() = delete;

    Index(const std::string& name,
          const uint32_t collection_id,
          const Store* store,
          SynonymIndex* synonym_index,
          ThreadPool* thread_pool,
          const tsl::htrie_map<char, field>& search_schema,
          const std::vector<char>& symbols_to_index,
          const std::vector<char>& token_separators);

    ~Index();

    static void concat_topster_ids(Topster<KV>*& topster, spp::sparse_hash_map<uint64_t, std::vector<KV*>>& topster_ids);

    static int64_t score_results2(const std::vector<sort_by> & sort_fields, const uint16_t & query_index,
                           const size_t field_id, const bool field_is_array, const uint32_t total_cost,
                           int64_t& match_score,
                           const uint32_t seq_id, const int sort_order[3],
                           const bool prioritize_exact_match,
                           const bool single_exact_query_token,
                           const bool prioritize_token_position,
                           size_t num_query_tokens,
                           int syn_orig_num_tokens,
                           const std::vector<posting_list_t::iterator_t>& posting_lists);

    static int64_t get_points_from_doc(const nlohmann::json &document, const std::string & default_sorting_field);

    const spp::sparse_hash_map<std::string, art_tree *>& _get_search_index() const;

    const spp::sparse_hash_map<std::string, num_tree_t*>& _get_numerical_index() const;

    const spp::sparse_hash_map<std::string, NumericTrie*>& _get_range_index() const;

    const spp::sparse_hash_map<std::string, array_mapped_infix_t>& _get_infix_index() const;

    const spp::sparse_hash_map<std::string, hnsw_index_t*>& _get_vector_index() const;

    facet_index_t* _get_facet_index() const;

    static int get_bounded_typo_cost(const size_t max_cost, const std::string& token, const size_t token_len,
                                     size_t min_len_1typo, size_t min_len_2typo,  bool enable_typos_for_numerical_tokens=true,
                                     bool enable_typos_for_alpha_numerical_tokens = true);

    static int64_t float_to_int64_t(float n);

    static float int64_t_to_float(int64_t n);

    void get_distinct_id(posting_list_t::iterator_t& facet_index_it, const uint32_t seq_id,
                         const bool is_array, const bool group_missing_values, uint64_t& distinct_id,
                         const bool& is_group_by_first_pass, std::set<uint32_t>& group_by_missing_value_ids,
                         bool is_reverse=false) const;

    void get_group_by_values(std::vector<std::vector<KV *>>& result_kvs,
                             std::vector<std::vector<KV *>>& override_result_kvs,
                             std::vector<group_by_field_it_t>& group_by_fields,
                             std::vector<std::set<std::string>>& group_by_values_list) const;

    static void compute_token_offsets_facets(index_record& record,
                                             const tsl::htrie_map<char, field>& search_schema,
                                             const std::vector<char>& local_token_separators,
                                             const std::vector<char>& local_symbols_to_index);

    static void tokenize_string_field(const nlohmann::json& document,
                                      const field& search_field, std::vector<std::string>& tokens,
                                      const std::string& locale,
                                      const std::vector<char>& symbols_to_index,
                                      const std::vector<char>& token_separators);

    std::string get_facet_str_val(const std::string& field_name, uint32_t facet_id);

    // Public operations

    Option<bool> run_search(search_args* search_params);

    Option<bool> search(std::vector<query_tokens_t>& field_query_tokens, const std::vector<search_field_t>& the_fields,
                const text_match_type_t match_type,
                std::unique_ptr<filter_node_t>& filter_tree_root, std::vector<facet>& facets, facet_query_t facet_query,
                const int max_facet_values,
                const std::vector<std::pair<uint32_t, uint32_t>>& included_ids,
                const std::vector<uint32_t>& excluded_ids, std::vector<sort_by>& sort_fields_std,
                const std::vector<uint32_t>& num_typos, Topster<KV>*& topster, Topster<KV>*& curated_topster,
                const size_t fetch_size,
                const size_t per_page,
                const size_t offset, const token_ordering token_order, const std::vector<bool>& prefixes,
                const size_t drop_tokens_threshold, size_t& all_result_ids_len,
                spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                std::vector<std::vector<art_leaf*>>& searched_queries,
                tsl::htrie_map<char, token_leaf>& qtoken_set,
                std::vector<std::vector<KV*>>& raw_result_kvs, std::vector<std::vector<KV*>>& override_result_kvs,
                const size_t typo_tokens_threshold, const size_t group_limit,
                const std::vector<std::string>& group_by_fields,
                const bool group_missing_values,
                const string& default_sorting_field, bool prioritize_exact_match,
                const bool prioritize_token_position, const bool prioritize_num_matching_fields,
                bool exhaustive_search,
                size_t concurrency, size_t search_cutoff_ms, size_t min_len_1typo, size_t min_len_2typo,
                size_t max_candidates, const std::vector<enable_t>& infixes, const size_t max_extra_prefix,
                const size_t max_extra_suffix, const size_t facet_query_num_typos,
                const bool filter_curated_hits, enable_t split_join_tokens,
                const vector_query_t& vector_query, size_t facet_sample_percent, size_t facet_sample_threshold,
                const drop_tokens_param_t drop_tokens_mode,
                const std::vector<facet_index_type_t>& facet_index_types,
                bool enable_typos_for_numerical_tokens,
                bool enable_synonyms,
                bool synonym_prefix,
                uint32_t synonym_num_typos,
                bool enable_lazy_filter,
                bool enable_typos_for_alpha_numerical_tokens,
                const size_t& max_filter_by_candidates,
                bool rerank_hybrid_matches, const bool& validate_field_names,
                bool is_group_by_first_pass,
                std::set<uint32_t>& group_by_missing_value_ids) const;

    void remove_field(uint32_t seq_id, nlohmann::json& document, const std::string& field_name,
                      const bool is_update);

    Option<uint32_t> remove(const uint32_t seq_id, nlohmann::json & document,
                            const std::vector<field>& del_fields, const bool is_update);

    static void validate_and_preprocess(Index *index, std::vector<index_record>& iter_batch,
                                          const size_t batch_start_index, const size_t batch_size,
                                          const std::string & default_sorting_field,
                                          const tsl::htrie_map<char, field> & search_schema,
                                          const tsl::htrie_map<char, field> & embedding_fields,
                                          const std::string& fallback_field_type,
                                          const std::vector<char>& token_separators,
                                          const std::vector<char>& symbols_to_index,
                                          const bool do_validation, const size_t remote_embedding_batch_size = 200,
                                          const size_t remote_embedding_timeout_ms = 60000, const size_t remote_embedding_num_tries = 2, const bool generate_embeddings = true);

    static size_t batch_memory_index(Index *index,
                                     std::vector<index_record>& iter_batch,
                                     const std::string& default_sorting_field,
                                     const tsl::htrie_map<char, field>& search_schema,
                                     const tsl::htrie_map<char, field> & embedding_fields,
                                     const std::string& fallback_field_type,
                                     const std::vector<char>& token_separators,
                                     const std::vector<char>& symbols_to_index,
                                     const bool do_validation, const size_t remote_embedding_batch_size = 200,
                                     const size_t remote_embedding_timeout_ms = 60000,
                                     const size_t remote_embedding_num_tries = 2, const bool generate_embeddings = true,
                                     const bool use_addition_fields = false,
                                     const tsl::htrie_map<char, field>& addition_fields = tsl::htrie_map<char, field>(),
                                     const std::string& collection_name = "",
                                     const spp::sparse_hash_map<std::string, std::set<reference_pair_t>>& async_referenced_ins =
                                            spp::sparse_hash_map<std::string, std::set<reference_pair_t>>());

    void index_field_in_memory(const std::string& collection_name, const field& afield,
                               std::vector<index_record>& iter_batch,
                               const std::set<reference_pair_t>& async_referenced_ins = {});

    template<class T>
    void iterate_and_index_numerical_field(std::vector<index_record>& iter_batch, const field& afield, T func);

    //static bool is_point_in_polygon(const Geofence& poly, const GeoCoord& point);

    //static double transform_for_180th_meridian(Geofence& poly);

    //static void transform_for_180th_meridian(GeoCoord& point, double offset);

    art_leaf* get_token_leaf(const std::string & field_name, const unsigned char* token, uint32_t token_len);

    Option<bool> do_filtering_with_lock(filter_node_t* const filter_tree_root,
                                        filter_result_t& filter_result,
                                        const std::string& collection_name = "",
                                        const bool& should_timeout = true,
                                        const bool& validate_field_names = true) const;

    Option<bool> do_reference_filtering_with_lock(filter_node_t* const filter_tree_root,
                                                  filter_result_t& filter_result,
                                                  const std::string& ref_collection_name,
                                                  const std::string& field_name,
                                                  negate_left_join_t& negate_left_join_info,
                                                  const bool& validate_field_names = true) const;

    Option<filter_result_t> do_filtering_with_reference_ids(const std::string& field_name,
                                                            const std::string& ref_collection_name,
                                                            filter_result_t&& ref_filter_result) const;

    void refresh_schemas(const std::vector<field>& new_fields, const std::vector<field>& del_fields);

    // the following methods are not synchronized because their parent calls are synchronized or they are const/static

    Option<bool> search_wildcard(filter_node_t const* const& filter_tree_root,
                                 const std::vector<sort_by>& sort_fields, Topster<KV>*& topster,
                                 spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                 std::vector<std::vector<art_leaf*>>& searched_queries, const size_t group_limit,
                                 const std::vector<std::string>& group_by_fields,
                                 const bool group_missing_values, const uint32_t* exclude_token_ids,
                                 size_t exclude_token_ids_size,
                                 uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                 filter_result_iterator_t* const filter_result_iterator,
                                 const size_t concurrency,
                                 const int* sort_order,
                                 std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                 const std::vector<size_t>& geopoint_indices,
                                 const bool& is_group_by_first_pass,
                                 std::set<uint32_t>& group_by_missing_value_ids) const;

    Option<bool> search_infix(const std::string& query, const std::string& field_name, std::vector<uint32_t>& ids,
                              size_t max_extra_prefix, size_t max_extra_suffix) const;

    void curate_filtered_ids(const uint32_t* exclude_token_ids, size_t exclude_token_ids_size, uint32_t*& filter_ids,
                             uint32_t& filter_ids_length, const std::vector<uint32_t>& curated_ids_sorted) const;

    Option<bool> populate_sort_mapping(int* sort_order, std::vector<size_t>& geopoint_indices,
                                       std::vector<sort_by>& sort_fields_std,
                                       std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                       const bool& validate_field_names) const;

    Option<bool> populate_sort_mapping_with_lock(int* sort_order, std::vector<size_t>& geopoint_indices,
                                                 std::vector<sort_by>& sort_fields_std,
                                                 std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                                 const bool& validate_field_names) const;

    int64_t reference_string_sort_score(const std::string& field_name, const uint32_t& seq_id) const;

    static void remove_matched_tokens(std::vector<std::string>& tokens, const std::set<std::string>& rule_token_set) ;

    void compute_facet_infos(const std::vector<facet>& facets, facet_query_t& facet_query,
                             const uint32_t facet_query_num_typos,
                             uint32_t* all_result_ids, const size_t& all_result_ids_len,
                             const std::vector<std::string>& group_by_fields,
                             size_t group_limit, bool is_wildcard_no_filter_query,
                             size_t max_candidates,
                             std::vector<facet_info_t>& facet_infos,
                             const std::vector<facet_index_type_t>& facet_index_types,
                             bool is_group_by_first_pass,
                             std::set<uint32_t>& group_by_missing_value_ids) const;

    void resolve_space_as_typos(std::vector<std::string>& qtokens, const std::string& field_name,
                                std::vector<std::vector<std::string>>& resolved_queries) const;

    size_t num_seq_ids() const;

    void handle_exclusion(const size_t num_search_fields, std::vector<query_tokens_t>& field_query_tokens,
                          const std::vector<search_field_t>& search_fields, uint32_t*& exclude_token_ids,
                          size_t& exclude_token_ids_size) const;

    Option<bool> do_infix_search(const size_t num_search_fields, const std::vector<search_field_t>& the_fields,
                                 const std::vector<enable_t>& infixes,
                                 const std::vector<sort_by>& sort_fields,
                                 std::vector<std::vector<art_leaf*>>& searched_queries, const size_t group_limit,
                                 const std::vector<std::string>& group_by_fields,
                                 const bool group_missing_values,
                                 const size_t max_extra_prefix,
                                 const size_t max_extra_suffix, const std::vector<token_t>& query_tokens, Topster<KV>* actual_topster,
                                 filter_result_iterator_t* const filter_result_iterator,
                                 const int sort_order[3],
                                 std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3> field_values,
                                 const std::vector<size_t>& geopoint_indices,
                                 const std::vector<uint32_t>& curated_ids_sorted,
                                 uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                 spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed, bool is_group_by_first_pass,
                                 std::set<uint32_t>& group_by_missing_value_ids) const;

    [[nodiscard]] Option<bool> do_synonym_search(const std::vector<search_field_t>& the_fields,
                                                 const text_match_type_t match_type,
                                                 filter_node_t const* const& filter_tree_root,
                                                 const std::vector<sort_by>& sort_fields_std,
                                                 const token_ordering& token_order,
                                                 const size_t typo_tokens_threshold, const size_t group_limit,
                                                 const std::vector<std::string>& group_by_fields,
                                                 const bool group_missing_values,
                                                 bool prioritize_exact_match,
                                                 const bool prioritize_token_position,
                                                 const bool prioritize_num_matching_fields,
                                                 const bool exhaustive_search, const size_t concurrency,
                                                 const std::vector<bool>& prefixes,
                                                 size_t min_len_1typo,
                                                 size_t min_len_2typo, const size_t max_candidates, const std::set<uint32_t>& curated_ids,
                                                 const std::vector<uint32_t>& curated_ids_sorted, const uint32_t* exclude_token_ids,
                                                 size_t exclude_token_ids_size,
                                                 Topster<KV>* actual_topster,
                                                 std::vector<std::vector<token_t>>& q_pos_synonyms,
                                                 int syn_orig_num_tokens,
                                                 spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                                 std::vector<std::vector<art_leaf*>>& searched_queries,
                                                 uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                                 filter_result_iterator_t* const filter_result_iterator,
                                                 std::set<uint64>& query_hashes,
                                                 const int* sort_order,
                                                 std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                                 const std::vector<size_t>& geopoint_indices,
                                                 tsl::htrie_map<char, token_leaf>& qtoken_set,
                                                 bool is_group_by_first_pass,
                                                 std::set<uint32_t>& group_by_missing_value_ids) const;

    Option<bool> do_phrase_search(const size_t num_search_fields, const std::vector<search_field_t>& search_fields,
                                  std::vector<query_tokens_t>& field_query_tokens,
                                  const std::vector<sort_by>& sort_fields,
                                  std::vector<std::vector<art_leaf*>>& searched_queries, const size_t group_limit,
                                  const std::vector<std::string>& group_by_fields,
                                  const bool group_missing_values,
                                  Topster<KV>* actual_topster,
                                  const int sort_order[3],
                                  std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3> field_values,
                                  const std::vector<size_t>& geopoint_indices,
                                  filter_result_iterator_t*& filter_result_iterator,
                                  uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                  spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                  const uint32_t* excluded_result_ids, size_t excluded_result_ids_size,
                                  bool is_wildcard_query, bool is_group_by_first_pass,
                                  std::set<uint32_t>& group_by_missing_value_ids) const;

    [[nodiscard]] Option<bool> fuzzy_search_fields(const std::vector<search_field_t>& the_fields,
                                                   const std::vector<token_t>& query_tokens,
                                                   const std::vector<token_t>& dropped_tokens,
                                                   const text_match_type_t match_type,
                                                   const uint32_t* excluded_result_ids,
                                                   size_t excluded_result_ids_size,
                                                   filter_result_iterator_t* const filter_result_iterator,
                                                   const std::vector<uint32_t>& curated_ids,
                                                   const std::vector<sort_by>& sort_fields,
                                                   const std::vector<uint32_t>& num_typos,
                                                   std::vector<std::vector<art_leaf*>>& searched_queries,
                                                   tsl::htrie_map<char, token_leaf>& qtoken_set,
                                                   Topster<KV>*& topster, spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                                   uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                                   const size_t group_limit, const std::vector<std::string>& group_by_fields,
                                                   const bool group_missing_values,
                                                   bool prioritize_exact_match,
                                                   const bool prioritize_token_position,
                                                   const bool prioritize_num_matching_fields,
                                                   std::set<uint64>& query_hashes,
                                                   const token_ordering token_order,
                                                   const std::vector<bool>& prefixes,
                                                   const size_t typo_tokens_threshold,
                                                   const bool exhaustive_search,
                                                   const size_t max_candidates,
                                                   size_t min_len_1typo,
                                                   size_t min_len_2typo,
                                                   int syn_orig_num_tokens,
                                                   const int* sort_order,
                                                   std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                                   const std::vector<size_t>& geopoint_indices,
                                                   bool is_group_by_first_pass,
                                                   std::set<uint32_t>& group_by_missing_value_ids,
                                                   bool enable_typos_for_numerical_tokens = true,
                                                   bool enable_typos_for_alpha_numerical_tokens = true) const;

    void find_across_fields(const token_t& previous_token,
                            const std::string& previous_token_str,
                            const std::vector<search_field_t>& the_fields,
                            const size_t num_search_fields,
                            filter_result_iterator_t* const filter_result_iterator,
                            const uint32_t* exclude_token_ids,
                            size_t exclude_token_ids_size,
                            std::vector<uint32_t>& prev_token_doc_ids,
                            std::vector<size_t>& top_prefix_field_ids) const;

    Option<bool> search_across_fields(const std::vector<token_t>& query_tokens,
                                      const std::vector<uint32_t>& num_typos,
                                      const std::vector<bool>& prefixes,
                                      const std::vector<search_field_t>& the_fields,
                                      const size_t num_search_fields,
                                      const text_match_type_t match_type,
                                      const std::vector<sort_by>& sort_fields,
                                      Topster<KV>*& topster,
                                      spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                      std::vector<std::vector<art_leaf*>>& searched_queries,
                                      tsl::htrie_map<char, token_leaf>& qtoken_set,
                                      const std::vector<token_t>& dropped_tokens,
                                      const size_t group_limit,
                                      const std::vector<std::string>& group_by_fields,
                                      const bool group_missing_values,
                                      bool prioritize_exact_match,
                                      const bool search_all_candidates,
                                      const bool prioritize_num_matching_fields,
                                      filter_result_iterator_t* const filter_result_iterator,
                                      const uint32_t total_cost,
                                      const int syn_orig_num_tokens,
                                      const uint32_t* excluded_result_ids,
                                      size_t excluded_result_ids_size,
                                      const int* sort_order,
                                      std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3>& field_values,
                                      const std::vector<size_t>& geopoint_indices,
                                      std::vector<uint32_t>& id_buff,
                                      size_t& num_keyword_matches,
                                      uint32_t*& all_result_ids, size_t& all_result_ids_len,
                                      bool is_group_by_first_pass,
                                      std::set<uint32_t>& group_by_missing_value_ids) const;

    static int64_t compute_aggregated_score(const std::vector<or_iterator_t>& its,
                                     std::vector<or_iterator_t>& dropped_token_its,
                                     const std::vector<search_field_t>& the_fields,
                                     const std::vector<token_t>& query_tokens,
                                     const size_t num_search_fields,
                                     const text_match_type_t match_type,
                                     const bool prioritize_exact_match,
                                     const bool psearch_acrossrioritize_token_position,
                                     const bool prioritize_num_matching_fields,
                                     const uint32_t total_cost,
                                     const int syn_orig_num_tokens,
                                     const uint32_t seq_id,
                                     const std::vector<sort_by>& sort_fields,
                                     const tsl::htrie_map<char, field>& search_schema,
                                     const std::vector<std::vector<art_leaf*>>& searched_queries,
                                     const int* sort_order,
                                     int64_t& out_best_field_match_score);

    void process_filter_overrides(const std::vector<const override_t*>& filter_overrides,
                                  std::vector<std::string>& query_tokens,
                                  token_ordering token_order,
                                  std::unique_ptr<filter_node_t>& filter_tree_root,
                                  std::vector<const override_t*>& matched_dynamic_overrides,
                                  nlohmann::json& override_metadata,
                                  bool enable_typos_for_numerical_tokens,
                                  bool enable_typos_for_alpha_numerical_tokens,
                                  const bool& validate_field_names = true) const;

    Option<bool> compute_sort_scores(const std::vector<sort_by>& sort_fields, const int* sort_order,
                                     std::array<spp::sparse_hash_map<uint32_t, int64_t, Hasher32>*, 3> field_values,
                                     const std::vector<size_t>& geopoint_indices, uint32_t seq_id,
                                     const std::map<basic_string<char>, reference_filter_result_t>& references,
                                     std::vector<uint32_t>& filter_indexes, int64_t max_field_match_score,
                                     int64_t* scores, int64_t& match_score_index, bool& should_skip,
                                     float vector_distance = 0) const;

    void process_curated_ids(const std::vector<std::pair<uint32_t, uint32_t>>& included_ids,
                             const std::vector<uint32_t>& excluded_ids,
                             const std::vector<std::string>& group_by_fields,
                             const size_t group_limit, const bool group_missing_values, const bool filter_curated_hits,
                             filter_result_iterator_t* const filter_result_iterator,
                             std::set<uint32_t>& curated_ids,
                             std::map<size_t, std::map<size_t, uint32_t>>& included_ids_map,
                             std::vector<uint32_t>& included_ids_vec,
                             bool is_group_by_first_pass,
                             std::set<uint32_t>& group_by_missing_value_ids) const;
    
    int64_t get_doc_val_from_sort_index(sort_index_iterator it, uint32_t doc_seq_id) const;

    Option<bool> seq_ids_outside_top_k(const std::string& field_name, size_t k,
                                       std::vector<uint32_t>& outside_seq_ids);

    Option<bool> get_related_ids(const std::string& field_name, const uint32_t& seq_id,
                                 std::vector<uint32_t>& result) const;

    Option<bool> get_object_array_related_id(const std::string& collection_name,
                                             const std::string& reference_helper_field_name,
                                             const uint32_t& seq_id, const uint32_t& object_index,
                                             uint32_t& result) const;

    Option<uint32_t> get_sort_index_value_with_lock(const std::string& field_name,
                                                    const uint32_t& seq_id) const;

    friend class filter_result_iterator_t;

    void repair_hnsw_index();

    void aggregate_facet(const size_t group_limit, facet& this_facet, facet& acc_facet) const;

    Option<int64_t> get_geo_distance_with_lock(const std::string& geo_field_name, const uint32_t& seq_id,
                                               const S2LatLng& reference_lat_lng, const bool& round_distance = false) const;

    Option<int64_t> get_referenced_geo_distance(const sort_by& sort_field, uint32_t seq_id,
                                                const std::map<basic_string<char>, reference_filter_result_t>& references,
                                                const S2LatLng& reference_lat_lng, const bool& round_distance = false) const;

    Option<uint32_t> get_ref_seq_id(const sort_by& sort_field, const uint32_t& seq_id,
                                    const std::map<std::string, reference_filter_result_t>& references,
                                    std::string& ref_collection_name) const;

    void get_top_k_result_ids(const std::vector<std::vector<KV*>>& raw_result_kvs, std::vector<uint32_t>& result_ids) const;

    void compute_aux_scores(Topster<KV>* topster, const std::vector<search_field_t>& the_fields,
                            const std::vector<token_t>& query_tokens, uint16_t search_query_size,
                            const std::vector<sort_by>& sort_fields_std, const int* sort_order,
                            const vector_query_t& vector_query, const text_match_type_t match_type,
                            bool prioritize_exact_match, const bool prioritize_token_position,
                            const bool prioritize_num_matching_fields) const;

    float compute_decay_function_score(const sort_by& sort_field, uint32_t seq_id) const;

    void get_field_token_its(const size_t num_search_fields, std::vector<art_leaf*>& query_suggestion,
                             std::vector<or_iterator_t>& token_its, std::vector<posting_list_t*>& expanded_plists,
                             const std::vector<token_t>& query_tokens,
                             const std::vector<search_field_t>& the_fields) const;

    static void populate_result_kvs(Topster<KV>* topster, std::vector<std::vector<KV *>> &result_kvs,
                                    const spp::sparse_hash_map<uint64_t, uint32_t>& groups_processed,
                                    const std::vector<sort_by>& sort_by_fields,
                                    const bool& is_group_by_first_pass);

    GeoPolygonIndex* get_geopolygon_index(const std::string& field_name) const;
};

template<class T>
void Index::iterate_and_index_numerical_field(std::vector<index_record>& iter_batch, const field& afield, T func) {
    for(auto& record: iter_batch) {
        if(!record.indexed.ok()) {
            continue;
        }

        const auto& document = record.doc;
        const auto seq_id = record.seq_id;

        if (document.count(afield.name) == 0 || !afield.index) {
            continue;
        }

        try {
            func(record, seq_id);
        } catch(const std::exception &e) {
            LOG(INFO) << "Error while indexing numerical field." << e.what();
            record.index_failure(400, e.what());
        }
    }
}
