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

namespace Gt {
internal class MockFileOutputStream : FileOutputStream {
    private MockFile file;
    private MemoryOutputStream memstream;

    public MockFileOutputStream(MockFile file, MemoryOutputStream memstream) {
        this.file = file;
        this.memstream = memstream;
    }

    public override ssize_t write([CCode(array_length_type = "gsize")] uint8[] buffer,
        Cancellable? cancellable = null) throws IOError
    {
        return memstream.write(buffer, cancellable);
    }

    public override bool close(Cancellable? cancellable = null) throws IOError {
        var retval = memstream.close(cancellable);
        var data_written = memstream.steal_as_bytes();
        file.contents = data_written;
        return retval;
    }

    public override FileInfo query_info(string attributes,
        Cancellable? cancellable = null) throws Error
    {
        return file.query_info(attributes, FileQueryInfoFlags.NONE, cancellable);
    }

    public override string get_etag() {
        // FIXME: etags are blank for now
        return "";
    }

    public override int64 tell() {
        return (memstream as Seekable).tell();
    }

    public override bool can_seek() {
        return (memstream as Seekable).can_seek();
    }

    public override bool seek(int64 offset, SeekType type,
        Cancellable? cancellable = null) throws Error
    {
        return (memstream as Seekable).seek(offset, type, cancellable);
    }

    public override bool can_truncate () {
        return (memstream as Seekable).can_truncate();
    }

    public override bool truncate_fn(int64 size,
        Cancellable? cancellable = null) throws Error
    {
        return (memstream as Seekable).truncate(size, cancellable);
    }

    // public override async FileInfo query_info_async(string attributes,
    //    int io_priority = Priority.DEFAULT, Cancellable? cancellable = null)
    //    throws Error;
}
}  // namespace Gt
