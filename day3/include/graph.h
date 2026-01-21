#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <map>
#include <stdint.h>
#include <deque>

// ============================================
// 图的顶点结构
// ============================================
struct GraphVertex {
    int indegree;                    // 入度（有多少条边指向我）
    std::vector<uint64_t> neighbors; // 出边列表（我指向谁）
    
    GraphVertex() : indegree(0) {}
};

// ============================================
// 有向图类（专门用于死锁检测）
// ============================================
class DirectedGraph {
public:
    DirectedGraph() {}
    
    // ========================================
    // 核心接口
    // ========================================
    
    // 添加一条边：from → to
    void add_edge(uint64_t from, uint64_t to);
    
    // 检测是否有环（返回 true 表示有环，即死锁）
    bool has_cycle();
    
    // 获取图中的所有节点ID（用于打印死锁信息）
    std::vector<uint64_t> get_all_nodes() const;
    
    // 清空图
    void clear();
    
    // 获取节点数量
    size_t size() const { return graph_.size(); }
    
    // ========================================
    // 调试接口
    // ========================================
    void print_graph() const;

private:
    // 图的邻接表表示：节点ID → 顶点信息
    std::map<uint64_t, GraphVertex> graph_;
    
    // 确保节点存在（如果不存在则创建）
    void ensure_node_exists(uint64_t node_id);
};

#endif // GRAPH_H