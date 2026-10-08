#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>

namespace switchmodel {
    class lpmtable{
        private:
        struct TrieNode{
            bool is_port{false};
            int outport_interf{-1};
            std::unique_ptr<TrieNode> left{nullptr};// bit 0
            std::unique_ptr<TrieNode> right{nullptr};// bit 1

        };
        std::unique_ptr<TrieNode>root;
        size_t node_cnt{1};
        public:
        lpmtable() : root(std::make_unique<TrieNode>()){}
        
        void insert(uint32_t prefix,uint8_t prefix_len,int out_interf){
            TrieNode* curr=root.get();
            for(int i=0;i<prefix_len;++i){
                bool bit =(prefix&(1U << (31-i)))!=0;
                if(!bit){
                    if(!curr->left){
                        curr->left = std::make_unique<TrieNode>();
                        node_cnt++;
                    }
                }else{
                   if(!curr->right){
                        curr->right = std::make_unique<TrieNode>();
                        node_cnt++;
                    }
                }
            }
            curr->is_port =true;
            curr->outport_interf=out_interf;
        }
        
        [[nodiscard]] int lookup(uint32_t dstip)const noexcept{
            const TrieNode* cur=root.get();
            int bstprt=-1;
             // default root 0.0.0.0/0
            if(cur->is_port){
                bstprt=cur->outport_interf;
            }
            for (int i = 0; i < 32; i++)
            {
                bool bit =(dstip&(1U<<(31-i)))!=0;
                if(!bit){
                    if(cur->left){
                        cur=cur->left.get();
                    }else{
                        break;
                    }
                }else{
                    if(cur->right){
                        cur=cur->right.get();
                    }else{
                        break;
                    }
                }
                if(cur->is_port){
                    bstprt=cur->outport_interf;
                }

            }
            return bstprt;
        }

        [[nodiscard]] size_t get_nod_cnt()const noexcept{
            return node_cnt;
        }

        //rst
        void clear(){
            root=std::make_unique<TrieNode>();
            node_cnt=1;
        }

    };
}