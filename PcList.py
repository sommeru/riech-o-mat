#!/usr/bin/env python
# -*- coding: utf-8 -*-

import wx


class PcList(wx.ListCtrl):
    """
    Widget displaying a punch card.
    """

    def __init__(self, parent):
        wx.ListCtrl.__init__(self, parent, -1, style=wx.LC_REPORT | wx.LC_VIRTUAL | wx.LC_HRULES | wx.LC_VRULES | wx.SUNKEN_BORDER| wx.LC_SINGLE_SEL)

        self._entries = []

        self._attrComment = wx.ListItemAttr()
        self._attrComment.SetTextColour("red")

        self._LINENO_COL = 0
        self._TIME_COL = 1
        self._FIRST_VALVE_COL = 2
        self._LAST_VALVE_COL = 6
        self._COMMENT_COL = 7

        self.InsertColumn(0, "Line", format=wx.LIST_FORMAT_RIGHT, width=64)
        self.InsertColumn(1, "Time", format=wx.LIST_FORMAT_RIGHT, width=96)
        self.InsertColumn(2, "Flow", format=wx.LIST_FORMAT_CENTER, width=48)
        self.InsertColumn(3, "Air", format=wx.LIST_FORMAT_CENTER, width=48)
        self.InsertColumn(4, "1", format=wx.LIST_FORMAT_CENTER, width=48)
        self.InsertColumn(5, "2", format=wx.LIST_FORMAT_CENTER, width=48)
        self.InsertColumn(6, "3", format=wx.LIST_FORMAT_CENTER, width=48)
        self.InsertColumn(7, "Comment", format=wx.LIST_FORMAT_LEFT, width=256)


    def setEntries(self, entries):
        self._entries = entries

        self.SetItemCount(len(self._entries))


    def getEntries(self):
        return self._entries


    def selectEntry(self, index):
        assert(index >= 0)
        assert(index < len(self._entries))
        
        self.Select(index)
        i = index + self.GetCountPerPage() - 1 - 3
        if i >= len(self._entries):
            i = len(self._entries) - 1
        self.EnsureVisible(i)
        self.EnsureVisible(index)


    def getSelectedEntry(self):
        item = self.GetFirstSelected()

        if item < 0:
            return None
        
        return self._entries[item]


    def OnGetItemText(self, item, col):
        e = self._entries[item]

        if col == self._LINENO_COL:
            return e.lineno
        elif col == self._TIME_COL:
            if e.valveStates:
                return "%.1f" % (e.time_ms / 1000.)
            else:
                return ""
        elif col >= self._FIRST_VALVE_COL and col <= self._LAST_VALVE_COL:
            if e.valveStates and col-self._FIRST_VALVE_COL < len(e.valveStates):
                return e.valveStates[col-self._FIRST_VALVE_COL]
            else:
                return ""
        elif col == self._COMMENT_COL:
            if e.comment:
                return e.comment
            else:
                return ""

        return "?"


    def OnGetItemAttr(self, item):
        e = self._entries[item]

        if not e.valveStates:
            return self._attrComment

        return None
