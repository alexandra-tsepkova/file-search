#include <cassert>
#include <dirent.h>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <unordered_set>

using namespace std;


struct TrieNode {
    void init_next() {
        for (int i = 0; i < 256; ++i) {
            next[i] = nullptr;
        }
    }

    bool is_terminal;
    int seq_id;
    TrieNode* next[256];
    TrieNode* suf_link = nullptr;
    TrieNode* parent = nullptr;
};

TrieNode* create_node(TrieNode* parent = nullptr) {
    auto node = new TrieNode({false, -1});
    node->init_next();
    node->parent = parent;
    return node;
}


TrieNode* parent(TrieNode* node) {
    return node->parent;
}


TrieNode* pi(TrieNode* node) {
    return node->suf_link;
}

TrieNode* delta(TrieNode* node, unsigned char byte) {
    if (node->next[byte] != nullptr) {
        return node->next[byte];
    } else {
        return delta(pi(node), byte);
    }
}


TrieNode* delta_and_store_entries(TrieNode* node, unsigned char byte, vector<pair<int, size_t>>& entries, size_t pos) {
    if (node->is_terminal) {
        entries.emplace_back(node->seq_id, pos);
    }

    if (node->next[byte] != nullptr) {
        return node->next[byte];
    } else {
        if (pi(node)->is_terminal) {
            entries.emplace_back(pi(node)->seq_id, pos);
        }
        return delta(pi(node), byte);
    }}


void build_suf_links_bfs(queue<TrieNode*>& q, unordered_set<TrieNode*>& visited) {
    auto current = q.front();

    q.pop();
    visited.insert(current);
    for (int i = 0; i < 256; ++i) {
        auto next = current->next[i];
        if (next == nullptr || visited.find(next) != visited.end()) {
            continue;
        } else {
            q.push(next);

            if (pi(current) != current) {
                next->suf_link = delta(pi(current), (unsigned char) (i));
            } else {
                next->suf_link = current;
            }
        }
    }
}


TrieNode* build_trie(const vector<string>& sequences) {
    auto root = create_node();

    int current_seq_id = 0;
    for (auto& seq : sequences) {
        auto current_node = root;
        for (unsigned char byte : seq) {
            assert( byte >= 0);

            if (current_node->next[byte] == nullptr) {
                current_node->next[byte] = create_node(current_node);
            }

            current_node = current_node->next[byte];
        }
        current_node->is_terminal = true;
        current_node->seq_id = current_seq_id;
        current_seq_id++;
    }

    root->suf_link = root;
    for (int i = 0; i < 256; i++) {
        if (root->next[i] == nullptr) {
            root->next[i] = root;
        }
    }

    queue<TrieNode*> q;
    unordered_set<TrieNode*> visited;
    q.push(root);
    while(!q.empty()) {
        build_suf_links_bfs(q, visited);
    }

    return root;
}


vector<pair<int, size_t>> find_entries(const string& target, const vector<string>& sequences) {
    vector<pair<int, size_t>> entries;
    auto root = build_trie(sequences);
    auto current = root;

    size_t pos = 0;
    for (unsigned char byte : target) {
        current = delta_and_store_entries(current, byte, entries, pos);
        pos++;
    }

    while (current != root) {
        if (current->is_terminal) {
            entries.emplace_back(current->seq_id, pos);
        }
        current = current->suf_link;
    }

    for (auto& elem : entries) {
        elem.second = elem.second - sequences[elem.first].size();
    }

    return entries;
}


vector<string> get_all_files_in_dir(string dir_path) {
    vector<string> result;
    DIR *dir = NULL;
    dirent *this_dir;
    dir = opendir(dir_path.c_str());
    if(!dir) {
        printf("Can't read directory with path %s\n", dir_path.c_str());
        exit(1);

    }
    else {
        while ((this_dir = readdir(dir)) != NULL) {
            char dirname[1024];
            result.emplace_back(this_dir->d_name);
        }
    }
    return result;
}


int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Not enough args");
        exit(1);
    }
    ostringstream target_stream;
    target_stream << ifstream(argv[1], ios::binary).rdbuf();
    string target(target_stream.str());

    vector<string> sequences;
    auto sequence_files = get_all_files_in_dir(argv[2]);
    for (auto file : sequence_files) {
        ostringstream stream;
        stream << ifstream(string(argv[2]) + "/" + file, ios::binary).rdbuf();
        string s(stream.str());
        if (!s.empty()) {
            sequences.push_back(s);
        }
    }

    auto results = find_entries(target, sequences);
    for (auto elem : results) {
        printf("%lu: %s\n", elem.second, sequences[elem.first].c_str());
    }
}