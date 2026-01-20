#include "graph.h"
#include <iostream>
#include <iomanip>

// ============================================
// 确保节点存在于图中
// ============================================
void DirectedGraph::ensure_node_exists(uint64_t node_id) {
    if (graph_.find(node_id) == graph_.end()) {
        graph_[node_id] = GraphVertex();
    }
}

// ============================================
// 添加一条有向边：from → to
// ============================================
void DirectedGraph::add_edge(uint64_t from, uint64_t to) {
    // 1. 确保两个节点都存在
    ensure_node_exists(from);
    ensure_node_exists(to);
    
    // 2. 添加边：from 的邻居列表加入 to
    graph_[from].neighbors.push_back(to);
    
    // 3. to 的入度 +1（因为有一条边指向它）
    graph_[to].indegree++;
}

// ============================================
// 检测是否有环（拓扑排序算法 - Kahn算法）
// ============================================
bool DirectedGraph::has_cycle() {
    if (graph_.empty()) {
        return false; // 空图没有环
    }
    
    // 复制一份入度数据（因为算法会修改入度）
    std::map<uint64_t, int> indegree_copy;
    for (const auto& pair : graph_) {
        indegree_copy[pair.first] = pair.second.indegree;
    }
    
    // Step 1: 找出所有入度为 0 的节点
    std::deque<uint64_t> queue;
    for (const auto& pair : indegree_copy) {
        if (pair.second == 0) {
            queue.push_back(pair.first);
        }
    }
    
    // Step 2: 拓扑排序主循环
    size_t processed_count = 0; // 已处理的节点数
    
    while (!queue.empty()) {
        // 取出一个入度为 0 的节点
        uint64_t node = queue.front();
        queue.pop_front();
        processed_count++;
        
        // 遍历该节点的所有邻居
        const std::vector<uint64_t>& neighbors = graph_[node].neighbors;
        for (size_t i = 0; i < neighbors.size(); i++) {
            uint64_t neighbor = neighbors[i];
            
            // 邻居的入度 -1
            indegree_copy[neighbor]--;
            
            // 如果邻居的入度变为 0，加入队列
            if (indegree_copy[neighbor] == 0) {
                queue.push_back(neighbor);
            }
        }
    }
    
    // Step 3: 判断是否有环
    // 如果处理的节点数 < 总节点数，说明有节点永远无法入队
    // → 这些节点在环中！
    return processed_count < graph_.size();
}

// ============================================
// 获取所有节点ID
// ============================================
std::vector<uint64_t> DirectedGraph::get_all_nodes() const {
    std::vector<uint64_t> nodes;
    for (const auto& pair : graph_) {
        nodes.push_back(pair.first);
    }
    return nodes;
}

// ============================================
// 清空图
// ============================================
void DirectedGraph::clear() {
    graph_.clear();
}

// ============================================
// 打印图结构（调试用）
// ============================================
void DirectedGraph::print_graph() const {
    std::cout << "\n========== Graph Structure ==========\n";
    std::cout << "Total nodes: " << graph_.size() << "\n";
    
    for (const auto& pair : graph_) {
        uint64_t node_id = pair.first;
        const GraphVertex& vertex = pair.second;
        
        std::cout << "Thread " << node_id 
                  << " (indegree=" << vertex.indegree << ")";
        
        if (!vertex.neighbors.empty()) {
            std::cout << " → [";
            for (size_t i = 0; i < vertex.neighbors.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << vertex.neighbors[i];
            }
            std::cout << "]";
        }
        std::cout << "\n";
    }
    std::cout << "====================================\n\n";
}