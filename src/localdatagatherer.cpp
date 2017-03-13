#include <set>

#include "localdatagatherer.h"


engine::LocalDataGatherer::LocalDataGatherer(support::ISpider* spider, IDataSource* ds):
        m_spider(spider), m_ds(ds)
{
}

engine::LocalDataGatherer::~LocalDataGatherer()
{
}

void
engine::LocalDataGatherer::compute_idf(const std::string& directory, idf_t& idf) const
{
        support::IDocumentIterator* doc_iter = m_spider->crawl(directory);
        while (doc_iter->has_next()) {
                // Collect terms.
                std::set<Term> terms;
                support::ITokenIterator* tok_iter = doc_iter->parse();
                while (tok_iter->has_next()) {
                        terms.insert(tok_iter->next());
                }
                delete tok_iter;

                // Update idf stats.
                for (Term term: terms) {
                        idf_t::iterator it = idf.find(term);
                        if (it != idf.end())	it->second ++;
                        else			idf.insert(idf_entry_t(term, 1));
                }
        }
        delete doc_iter;
}

void
engine::LocalDataGatherer::run(const std::string& directory)
{
        idf_t idf;
        compute_idf(directory, idf);

        // Gather terms and put into data source.
        support::IDocumentIterator* doc_iter = m_spider->crawl(directory);
        while (doc_iter->has_next()) {
                std::string descriptor = doc_iter->get_descriptor();
                Document curr_doc(descriptor, "Unknown", 0.0f);

                // Collect term frequency
                support::ITokenIterator* tok_iter = doc_iter->parse();
                std::map<Term, unsigned> terms_freq;
                while (tok_iter->has_next()) {
                        Term term = tok_iter->next();
                        std::map<Term, unsigned>::iterator it = terms_freq.find(term);
                        if (it != terms_freq.end())	it->second ++;
                        else				terms_freq.insert(std::pair<Term, unsigned>(term, 1));
                }
                delete tok_iter;

                // Put terms into document and save into data source.
                for (std::map<Term, unsigned>::const_iterator it = terms_freq.begin(); it != terms_freq.end(); ++ it) {
                        Term term = it->first;
                        term.set_tf(it->second);
                        term.set_idf(idf.at(term));
                        curr_doc.add_term(term);
                }

                std::vector<Document> docs;
                docs.push_back(curr_doc);
                m_ds->add_documents(docs);
        }
        m_ds->force_transaction();
        delete doc_iter;
}