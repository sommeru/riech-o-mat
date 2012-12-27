#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import time
import threading
import wx
import subprocess

class PcPlayer():
    """
    Helper class to create a thread to play the punch card.
    """
    
    class PcPlayerThread(threading.Thread):
        """
        Thread that plays the punch card.
        """
    
        def __init__(self, parent, entries, stepLength, startIndex = 0):
            super(PcPlayer.PcPlayerThread, self).__init__()
            self._stop = threading.Event()
            self._parent = parent
            self._entries = entries
            self._stepLength = stepLength
            self._currentIndex = startIndex
            self._startTime = None
            self._stepCount = None
            
        
        def run(self):
            
            self._startTime = time.time()
            self._stepCount = 0
            
            wx.CallAfter(self._parent._startPlayingCallback)
            while not self.stopped():
                while (self._currentIndex >= len(self._entries)) or not self._entries[self._currentIndex].valveStates:
                    if self._currentIndex < len(self._entries) - 1:
                        self._currentIndex += 1
                    else:
                        self.stop()
                        return
                
                valveStates = self._entries[self._currentIndex].valveStates
                
                self._changeValves(valveStates)
                wx.CallAfter(self._parent._currentIndexChangedCallback, self._currentIndex)
                wx.CallAfter(self._parent._valveStateChangedCallback, valveStates)
                
                self._stepCount += 1
                
                timeDeltaIs = time.time() - self._startTime
                timeDeltaShould = self._stepCount * self._stepLength
                timeDelta = timeDeltaShould - timeDeltaIs
                
                if timeDelta > 0:
                    time.sleep(timeDelta)
                
                if self._currentIndex < len(self._entries) - 1:
                    self._currentIndex += 1
                else:
                    self.stop()
                    return
    
    
        def stop(self):
            wx.CallAfter(self._parent._stopPlayingCallback)
            self._stop.set()
    
    
        def stopped(self):
            return self._stop.isSet()
        
        
        def _isvalveStatesAllowed(self, valveStates):
            if len(valveStates) < 2:
                return True
            
            if valveStates[0] == 0:
                return True
            
            for s in valveStates[1:]:
                if s == 1:
                    return True
            
            return False
            

        def _changeValves(self, valveStates):
            if not self._isvalveStatesAllowed(valveStates):
                wx.CallAfter(self._parent._errorCallback, caption="Unable to set valve states", message="This valve state was prohibited by software.")
                self.stop()
                return
    
            basedir = os.path.dirname(__file__)
            backend = os.path.join(basedir, 'riech-o-mat-backend')
            parameters = "".join(str(i) for i in valveStates)
            try:
                p = subprocess.Popen([backend, parameters], stderr=subprocess.PIPE)
                stderr = p.communicate()[1]
                if p.returncode != 0:
                    wx.CallAfter(self._parent._errorCallback, caption="Unable to set valve states", message="Backend returned error: %s" % (stderr))
                    self.stop()
            except OSError as e:
                wx.CallAfter(self._parent._errorCallback, caption="Unable to set valve states", message="Error running backend (%s): %s" % (backend, e.strerror))
                self.stop()


    def __init__(self):
        self._playerThread = None

        self._startPlayingCallback = None
        self._stopPlayingCallback = None
        self._valveStateChangedCallback = None
        self._currentIndexChangedCallback = None
        self._errorCallback = None


    def isPlaying(self):
        if not self._playerThread:
            return False
        
        return self._playerThread.is_alive()


    def setStartCallback(self, startPlayingCallback):
        self._startPlayingCallback = startPlayingCallback
    
    
    def setStopCallback(self, stopPlayingCallback):
        self._stopPlayingCallback = stopPlayingCallback


    def setValveStateChangedCallback(self, valveStateChangedCallback):
        self._valveStateChangedCallback = valveStateChangedCallback

        
    def setCurrentIndexChangedCallback(self, currentIndexChangedCallback):
        self._currentIndexChangedCallback = currentIndexChangedCallback

        
    def setErrorCallback(self, errorCallback):
        self._errorCallback = errorCallback


    def start(self, entries, stepLength, startIndex = 0):
        if self.isPlaying():
            return
        
        self._playerThread = self.PcPlayerThread(self, entries, stepLength, startIndex)
        self._playerThread.start()
            

    def stop(self):
        if not self.isPlaying():
            return
        self._playerThread.stop()
        self._playerThread.join()

            
    
