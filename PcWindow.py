#!/usr/bin/env python
# -*- coding: utf-8 -*-

import wx
import os
from PcList import PcList
from PcEntry import PcEntry
from PcPlayer import PcPlayer


class PcWindow(wx.Frame):
    """
    This is the main window of the riech-o-mat.
    """


    def __init__(self, *args, **kwds):

        self._valveState = []

        kwds["style"] = wx.DEFAULT_FRAME_STYLE
        wx.Frame.__init__(self, *args, **kwds)
        self.w_statusbar = self.CreateStatusBar(1, 0)

        self.w_toolbar = wx.ToolBar(self, -1, style=wx.TB_HORIZONTAL | wx.TB_TEXT)
        self.SetToolBar(self.w_toolbar)
        self._openBtn = self.w_toolbar.AddLabelTool(wx.ID_OPEN, "Open", wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN), wx.NullBitmap, wx.ITEM_NORMAL, "", "")
        self.Bind(wx.EVT_TOOL, self._onOpenBtn, id=wx.ID_OPEN)
        self.w_toolbar.AddSeparator()
        self._playBtn = self.w_toolbar.AddLabelTool(wx.ID_FORWARD, "Play / Stop", wx.ArtProvider.GetBitmap(wx.ART_GO_FORWARD), wx.NullBitmap, wx.ITEM_CHECK, "", "")
        self.Bind(wx.EVT_TOOL, self._togglePlaying, id=wx.ID_FORWARD)

        panel = wx.Panel(self)

        data = """\

      Riech-O-Mat 2.0

      Shortcuts:

      Enter: Play/Stop
      Ctrl+L: set step length
      Ctrl+O: open new punchcard

        """
        self.w_panel_help = wx.StaticText(panel, -1, data, pos=(20,15))


        sc = wx.SpinCtrl(panel, -1, "", (30, 50))
        sc.SetRange(50, 30000)
        sc.SetValue(1000)
        self.Bind(wx.EVT_SPINCTRL, self._onSpin, sc)
        self.w_stepLength = sc

        pb = wx.ToggleButton(panel, -1, "Play / Stop")
        self.Bind(wx.EVT_TOGGLEBUTTON, self._togglePlaying, pb)
        self.w_playButton = pb

        self.w_panel_status = wx.Panel(panel)
        self.w_panel_status.BackgroundStyle = wx.BG_STYLE_CUSTOM
        self.w_panel_status.Bind(wx.EVT_PAINT, self.OnPaint)

        self.w_punchcard = PcList(panel)
        self.w_punchcard.setEntries([])
        self.w_punchcard.SetFocus()
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self._onPcEntryActivated, id=self.w_punchcard.GetId())

        self.pcplayer = PcPlayer()
        self.pcplayer.setValveStateChangedCallback(self._onValveStatesChanged)
        self.pcplayer.setCurrentIndexChangedCallback(self._changeCurrentIndex)
        self.pcplayer.setStartCallback(self._onStartPlaying)
        self.pcplayer.setStopCallback(self._onStopPlaying)
        self.pcplayer.setErrorCallback(self._showError)
        
        self._onSpin(None)

        basedir = os.path.dirname(__file__)
        fname = os.path.join(basedir, 'riech-o-mat.ico')
        icons = wx.IconBundleFromFile(fname, wx.BITMAP_TYPE_ANY)
        self.SetIcons(icons)
        self.SetTitle("Riech-O-Mat")

        self.w_statusbar.SetStatusWidths([-1])
        w_statusbar_fields = [""]
        for i in range(len(w_statusbar_fields)):
            self.w_statusbar.SetStatusText(w_statusbar_fields[i], i)

        self.w_toolbar.Realize()

        self.w_panel_status.SetMinSize((320,240))

        w_sizer_all = wx.BoxSizer(wx.VERTICAL)
        w_sizer_main = wx.BoxSizer(wx.HORIZONTAL)
        w_sizer_left = wx.BoxSizer(wx.VERTICAL)
        w_sizer_steplen = wx.BoxSizer(wx.HORIZONTAL)
        w_sizer_steplen.Add(wx.StaticText(panel, -1, " step length: "), 0, wx.CENTER, 0)
        w_sizer_steplen.Add(self.w_stepLength, 1, wx.EXPAND, 0)
        w_sizer_steplen.Add(wx.StaticText(panel, -1, " ms "), 0, wx.CENTER, 0)
        w_sizer_steplen.Add(self.w_playButton, 0, wx.EXPAND, 0)
        w_sizer_left.Add(self.w_panel_help, 1, wx.EXPAND, 0)
        w_sizer_left.Add(w_sizer_steplen, 0, wx.EXPAND, 0)
        w_sizer_left.Add(self.w_panel_status, 0, wx.EXPAND, 0)
        w_sizer_main.Add(w_sizer_left, 0, wx.EXPAND, 0)
        w_sizer_main.Add(self.w_punchcard, 1, wx.EXPAND, 0)
        w_sizer_all.Add(w_sizer_main, 1, wx.EXPAND, 0)
        panel.SetSizer(w_sizer_all)
        w_sizer_all.Fit(self)
        self.Layout()
        self.Maximize()

        idEnter = wx.NewId()
        idFocusStepLength = wx.NewId()
        idAccelOpen = wx.NewId()
        aTable = wx.AcceleratorTable([
            (wx.ACCEL_NORMAL, wx.WXK_RETURN, idEnter),
            (wx.ACCEL_CTRL, ord('l'), idFocusStepLength),
            (wx.ACCEL_CTRL, ord('o'), idAccelOpen),
            ])
        self.SetAcceleratorTable(aTable)
        self.Bind(wx.EVT_MENU, self._togglePlaying, id=idEnter)
        self.Bind(wx.EVT_MENU, self._focusStepLength, id=idFocusStepLength)
        self.Bind(wx.EVT_MENU, self._onOpenBtn, id=idAccelOpen)

        basedir = os.path.dirname(__file__)
        fname = os.path.join(basedir, 'punchcard.txt')
        if os.path.exists(fname):
            self.loadPunchcard(fname)


    def __del__(self):
        if self.pcplayer.isPlaying():
            self.pcplayer.stop()
        

    def _calcTimes(self, entries, stepLength_ms):
        t = 0
        for e in entries:
            e.time_ms = t
            if not e.valveStates:
                continue
            t += stepLength_ms
            
        return entries


    def loadPunchcard(self, fname):
        entries = []

        f = open(fname, 'r')
        lineno = 0
        for line in f:
            lineno += 1
            line = line.strip()
            (valves, _dummy, comment) = line.partition('#')

            valves = valves.strip()
            if valves:
                valves = [(0 if (s == "0") else 1) for s in list(valves)]
                if len(valves) < 2:
                    entries.append(PcEntry(lineno, comment="ERROR: unparseable"))
                    continue
            else:
                valves = None
                
            comment = comment.strip()
            if comment:
                pass
            else: comment = None

            entries.append(PcEntry(lineno, valveStates=valves, comment=comment))


        f.close()

        entries = self._calcTimes(entries, self.w_stepLength.GetValue())
        self.w_punchcard.setEntries(entries)
        
        if len(entries) > 0:
            self.w_punchcard.Select(0)
            self.w_punchcard.EnsureVisible(0)


    def OnPaint(self, _event):
        dc = wx.PaintDC(self.w_panel_status)

        basedir = os.path.dirname(__file__)

        fname = os.path.join(basedir, 'valvestate-base.png')
        bmp = wx.Bitmap(fname)
        dc.DrawBitmap(bmp, 0, 0, True)

        for i in xrange(0, len(self._valveState)):
            v = self._valveState[i]
            fname2 = 'valvestate-%d-closed.png' % (i)
            if v != 0:
                fname2 = 'valvestate-%d-open.png' % (i)
            fname = os.path.join(basedir, fname2)
            bmp = wx.Bitmap(fname)
            dc.DrawBitmap(bmp, 0, 0, True)


    def _focusStepLength(self, _event):
        self.w_stepLength.SetFocus()


    def _onSpin(self, _event):
        entries = self._calcTimes(self.w_punchcard.getEntries(), self.w_stepLength.GetValue())
        self.w_punchcard.setEntries(entries)


    def _updateButtonStates(self):
        self.w_toolbar.ToggleTool(self._playBtn.GetId(), self.pcplayer.isPlaying())
        self.w_playButton.SetValue(self.pcplayer.isPlaying())


    def _onOpenBtn(self, _event):
        if self.pcplayer.isPlaying():
            return

        wildcard = "Punchcard file (*.txt)|*.txt|All files (*.*)|*.*"
        dlg = wx.FileDialog(self, message="Choose a punch card file", wildcard=wildcard, style=wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            paths = dlg.GetPaths()
            if len(paths) == 1:
                self.loadPunchcard(paths[0])
        dlg.Destroy()


    def _togglePlaying(self, event):
        if not self.pcplayer.isPlaying():
            index = self.w_punchcard.GetFirstSelected()
            if index < 0:
                index = 0
            self._onSpin(event)
            self.pcplayer.start(self.w_punchcard.getEntries(), self.w_stepLength.GetValue() / 1000., startIndex=index)
        else:
            self.pcplayer.stop()
            
            
    def _showError(self, caption, message):
        dlg = wx.MessageDialog(parent=None, caption=caption, message=message, style=wx.OK | wx.ICON_EXCLAMATION)
        dlg.ShowModal()
        dlg.Destroy()


    def _onValveStatesChanged(self, valveStates):
        self._valveState = valveStates
        self.w_panel_status.Refresh()


    def _changeCurrentIndex(self, currentIndex):
        self.w_punchcard.selectEntry(currentIndex)

        
    def _onPcEntryActivated(self, _event):
        entry = self.w_punchcard.getSelectedEntry()

        if entry and entry.valveStates:
            pass
                    

    def _onStartPlaying(self):
        self._updateButtonStates()
        self.w_stepLength.Disable()
        self.w_punchcard.Disable()
        self.w_playButton.SetFocus()
                    

    def _onStopPlaying(self):
        self._updateButtonStates()
        self.w_stepLength.Enable()
        self.w_punchcard.Enable()
        self.w_punchcard.SetFocus()
