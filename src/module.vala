/*
 * Copyright 2014, 2015 Philip Chimento <philip.chimento@gmail.com>
 *
 * This file is part of Gt.
 *
 * Gt is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Gt is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Gt. If not, see <http://www.gnu.org/licenses/>.
 */

// The funny namespace is because these functions must be called g_io_module_*
namespace G.Io {
public static void module_load(IOModule module) {
    IOExtensionPoint.implement(IOExtensionPoint.VFS, typeof(Gt.MockVfs), "mock", 10);
}

public static void module_unload(IOModule module) {
}

[CCode(array_null_terminated = true, array_length = false)]
public static string[] module_query() {
    return { IOExtensionPoint.VFS };
}
}  // namespace G.Io
