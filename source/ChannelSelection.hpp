//
//  ╔═╗┬ ┬┌┬┐┬┌─┐╔╦╗┬─┐┌─┐┬┌┬┐┌─┐
//  ╠═╣│ │ ││││ │ ║ ├┬┘├─┤│ │ └─┐
//  ╩ ╩└─┘─┴┘┴└─┘ ╩ ┴└─┴ ┴┴ ┴ └─┘
//
//  © 2021 Lorenz Bucher - all rights reserved

#pragma once

#include <set>
#include <vector>
#include <numeric>
#include <utility>

#include "Utils.hpp"

namespace slb
{

class SelectionItem
{
public:
    /** Discrete (deliberately non-explicit ctor) */
    SelectionItem(int channel) : m_selection({channel})
    {
        SLB_ASSERT(channel > 0, "invalid channel!");
    }
    
    /** Range
     TODO: consider making this explicit -- maybe rename class to something shorter in this case
     */
    SelectionItem(int first, int last) : m_selection(last - first + 1)
    {
        SLB_ASSERT(first > 0 && last >= first, "invalid range!");
        std::iota(m_selection.begin(), m_selection.end(), first);
    }
    
    std::set<int> get() const { return { m_selection.begin(), m_selection.end() }; }
    int size() const { return static_cast<int>(m_selection.size()); }

private:
    std::vector<int> m_selection;
};


class ChannelSelection
{
public:
    ChannelSelection(std::initializer_list<SelectionItem> selectionItems) : m_selectionItems(selectionItems) {}
    
    std::set<int> get() const
    {
        std::set<int> result;
        for (auto& item : m_selectionItems) {
            std::set<int> itemSelection = item.get();
            result.insert(itemSelection.begin(), itemSelection.end());
        }
        return result;
    }
    
private:
    std::vector<SelectionItem> m_selectionItems;

};


} // namespace slb
