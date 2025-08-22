#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LinkerHand Python SDK
灵巧手控制API
"""

from .linker_hand_api import LinkerHandApi

# 为了向后兼容，提供LinkerHandAPI别名
LinkerHandAPI = LinkerHandApi

__all__ = ['LinkerHandApi', 'LinkerHandAPI']