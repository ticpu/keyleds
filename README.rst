===============
|logo| keyleds
===============

Advanced RGB animation service for Logitech keyboards.

|animation|

.. note::
   Maintained fork of `spectras/keyleds`_, which has seen no activity since 2021.
   Adds Wayland support and build fixes for current toolchains. Bug reports are
   welcome on this repository's tracker; the upstream wiki is still the reference
   for configuration.

Quick links:

* `installing`_
* `documentation`_
* `sample configuration`_
* `issue tracker`_

This project supports all Logitech RGB keyboards, on all keyboard layouts. If yours doesn't
work it's a bug, open a ticket.

Features
--------

* Flexible per-application RGB settings with `key groups`_.
* Runs on X11 and Wayland: key events are read straight from the keyboard's
  event devices, so effects react no matter which window has focus.
* Reacts to window title changes, enabling switching profiles based on
  current webpage in browser or open file extension in editors. This needs an
  X display, so under Wayland it only follows XWayland clients.
* Improved, fully configurable animation plugins:

  - **Keypress feedback** effect.
  - **Fixed** colors.
  - **Breathing** effect.
  - **Wave** and **cycle** effect.
  - **Stars** effect.
  - **Idle dimming** effect.

* **Script your own effects** with the `LUA engine`_. You can even make on-keyboard games.

* Mix and match several effects to build complex animations.

* Multi-user, multi-keyboard support with per-user and per-keyboard configuration.

And a few goodies:

* **DBUS Interface** for scripting and richer interactions with your LUA effects.
* **Command-line tool** for your scripting needs and extended configuration
  (set game-mode keys, change report rate, see USB exchanges…).

----

Feedback, feature ideas, pull requests are welcome!

.. _spectras/keyleds: https://github.com/keyleds/keyleds
.. _installing: https://github.com/keyleds/keyleds/wiki/Installing
.. _documentation: https://github.com/keyleds/keyleds/wiki
.. _sample configuration: https://github.com/ticpu/keyleds/blob/main/keyledsd/keyledsd.conf.sample
.. _issue tracker: https://github.com/ticpu/keyleds/issues
.. _key groups: https://github.com/keyleds/keyleds/wiki/Key-Group
.. _LUA engine: https://github.com/keyleds/keyleds/wiki/LUA-Introduction
.. |logo| image:: logo.svg
   :width: 64px
   :height: 80px
   :align: middle
   :alt:
.. |animation| image:: animation.gif
   :width: 320px
   :height: 128px

