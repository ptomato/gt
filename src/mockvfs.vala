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

internal class MockVfs : Vfs {
    private static const string URI_SCHEME = "gt-mock";

    public override bool is_active() {
        return true;
    }

    public override File get_file_for_path(string path) {
        return new Gt.MockFile() as File;
    }

    public override File get_file_for_uri(string uri) {
        var id = uri[(URI_SCHEME + "://").length:uri.length];
        if ("#" in id)
            id = id[id.index_of_char('#'):id.length];

        return new Gt.MockFile.with_id(id);
    }

    public override File parse_name(string name) {
        return get_file_for_uri(name);
    }

    [CCode (array_length = false, array_null_terminated = true)]
    public override unowned string[] get_supported_uri_schemes() {
        const string uri_schemes[] = { URI_SCHEME, null };
        return uri_schemes;
    }
}
