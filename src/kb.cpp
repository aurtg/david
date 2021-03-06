#include "./kb.h"

namespace dav
{

namespace kb
{

std::mutex feature_to_rules_cdb_t::ms_mutex;


void feature_to_rules_cdb_t::prepare_compile()
{
	cdb_data_t::prepare_compile();
	m_feat2rids.clear();
}


void feature_to_rules_cdb_t::finalize()
{
    if (is_writable())
    {
        char key[256];

        for (const auto &p : m_feat2rids)
        {
            const conjunction_template_t &feat = p.first.first;
            is_backward_t backward = p.first.second;
            binary_writer_t wr_key(key, 256);

            wr_key.write<conjunction_template_t>(feat);
            wr_key.write<char>(backward ? 1 : 0);

            std::vector<char> val(sizeof(size_t) + sizeof(rule_id_t) * p.second.size(), '\0');
            binary_writer_t wr_val(&val[0], val.size());

            wr_val.write<size_t>(p.second.size());
            for (const auto &rid : p.second)
                wr_val.write<rule_id_t>(rid);

            put(key, wr_key.size(), &val[0], wr_val.size());
        }
    }

	cdb_data_t::finalize();
}


std::list<rule_id_t> feature_to_rules_cdb_t::gets(
	const conjunction_template_t &feat, is_backward_t backward) const
{
	assert(is_readable());

	std::list<rule_id_t> out;
	char key[512];
	binary_writer_t key_writer(key, 512);

	key_writer.write<conjunction_template_t>(feat);
	key_writer.write<char>(backward ? 1 : 0);

	size_t value_size;
    const char *value;

    {
        std::lock_guard<std::mutex> lock(ms_mutex);
        value = (const char*)get(key, key_writer.size(), &value_size);
    }

	if (value != nullptr)
	{
		binary_reader_t val_reader(value, value_size);
		size_t num = val_reader.get<size_t>();

		for (size_t i = 0; i < num; ++i)
			out.push_back(val_reader.get<rule_id_t>());
	}

	return out;
}


void feature_to_rules_cdb_t::insert(const conjunction_t &conj, is_backward_t backward, rule_id_t rid)
{
    auto feat = conj.feature();

    if (not feat.empty())
    	m_feat2rids[std::make_pair(conj.feature(), backward)].insert(rid);
}


void feature_to_rules_cdb_t::insert(const rule_t &r)
{
	insert(r.evidence(false), false, r.rid());
	insert(r.evidence(true), true, r.rid());
}


}

}