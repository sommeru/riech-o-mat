#!/usr/bin/env python
# -*- coding: utf-8 -*-

class PcEntry():
    """
    This represents a single line of a punch card.
    Each line can contain new valve states and/or a comment
    """
    
    def __init__(self, lineno, valveStates = None, comment = None):
        self.lineno = lineno
        self.time_ms = None
        self.valveStates = valveStates
        self.comment = comment
