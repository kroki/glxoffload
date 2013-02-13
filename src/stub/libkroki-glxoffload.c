/*
  Copyright (C) 2013 Tomash Brechko.  All rights reserved.

  This file is part of kroki/glxoffload.

  Kroki/glxoffload is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Kroki/glxoffload is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with kroki/glxoffload.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  This file is intentionally left blank.  To provide natural access to
  OpenGL symbols (both direct and with dlsym()) from our libGL we'd
  like to link with accelerated libGL.  However it's not possible
  because both libraries have the same soname.  So we link with
  libkroki-glxoffload instead in order to add a proper DT_NEEDED
  record to our libGL ELF object and at runtime instead of
  libkroki-glxoffload we substitute a symlink pointing to the
  accelerated libGL.
*/
