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
public class MockFile : Object, File {
    private static const string URI_SCHEME = "gt-mock";

    /* Members */

    private string id;
    private string? basename;  // UTF-8, same as display name
    // MockFile keeps references to its parent file and its direct children
    private MockFile? ancestor;
    private List<MockFile> children;

    /* Constructors */

    public MockFile() {}

    public MockFile.with_id(string id) {
        this.id = id;
    }

    construct {
        id = Checksum.compute_for_string(ChecksumType.MD5, "%p".printf(this), -1);
    }

    /* GFile implementations */

    public File dup() {
        return this;
    }

    public uint hash() {
        return direct_hash(this);
    }

    public bool equal(File other) {
        return other is MockFile && id == (other as MockFile).id;
    }

    // Checks to see if a file is native to the system. Mock files are not.
    public bool is_native() {
        return false;
    }

    public bool has_uri_scheme(string uri_scheme) {
        return uri_scheme == URI_SCHEME;
    }

    public string get_uri_scheme() {
        return URI_SCHEME;
    }

    public string? get_basename() {
        return basename ?? id;
    }

    public string? get_path() {
        return null;  // Spec: 'returns NULL if no such path exists'
    }

    // FIXME: this URI doesn't really mean anything
    public string get_uri() {
        return URI_SCHEME + "://" + id;
    }

    // The parse name is the same as the URI.
    public string get_parse_name() {
        return get_uri();
    }

    private static void associate_parent_with_child(MockFile parent, MockFile child) {
        parent.children.prepend(child);
        if (child.ancestor != null)
            critical("Bookkeeping failure in GMockFile");
        child.ancestor = parent;
    }

    // If the mock file was created through g_file_get_child() or similar,
    // then it should already have a parent. Otherwise, we can create
    // parents infinitely.
    public File? get_parent() {
        if (ancestor == null) {
            var parent = new MockFile();
            associate_parent_with_child(parent, this);
            return parent;
        }
        return ancestor;
    }

    // Helper function: returns the number of path components (including the
    // filename of @descendant itself) in the hierarchy between @descendant
    // and @parent. Returns -1 if @parent is not an ancestor of @descendant.
    private static int match_prefix(MockFile parent, MockFile descendant) {
        var count = 0;
        for (var file = descendant; file != null; file = file.ancestor, count++) {
            if (file.id == parent.id)
                return count;
        }
        return -1;
    }

    // Must be declared before has_prefix() so it overwrites the prefix_matches
    // vfunc pointer in the class structure.
    [Deprecated(replacement = "has_prefix", since = "vala-0.16")]
    [NoWrapper]
    public bool prefix_matches(File file) {
        return has_prefix(file);
    }
    [CCode(vfunc_name = "prefix_matches")]
    public bool has_prefix(File file) {
        return file is MockFile && match_prefix(this, file as MockFile) != -1;
    }

    public string? get_relative_path(File descendant) {
        if (!(descendant is MockFile))
            return null;
        var descendant_mock = descendant as MockFile;
        int path_component_count = match_prefix(this, descendant_mock);
        if (path_component_count == -1)
            return null;  // @descendant is not a descendant of this
        if (path_component_count == 0)
            return ".";  // same file

        string[] components = new string[path_component_count];
        components[--path_component_count] = descendant_mock.basename;
        for (var file = descendant_mock.ancestor; path_component_count > 0;
            file = file.ancestor, path_component_count--) {
            components[--path_component_count] = file.basename;
        }
        // There's no Path.build_filenamev() in Vala
        return string.joinv(Path.DIR_SEPARATOR_S, components);
    }

    // Helper function: Returns a child (transfer full) if one exists with
    // @basename
    private MockFile? get_child_with_basename(string basename) {
        foreach (var file in children) {
            if (file.basename == basename) {
                return file;
            }
        }
        return null;
    }

    // A mock file has no path, so just return a new mock file
    public File resolve_relative_path(string relative_path) {
        if (Path.is_absolute(relative_path))
            return new MockFile();
        if (relative_path == "")
            return this;

        var paths = relative_path.split(Path.DIR_SEPARATOR_S);
        MockFile? child = null;
        MockFile parent = this;
        foreach (unowned string iter in paths) {
            if (iter == "..") {
                parent = parent.get_parent() as MockFile;
                continue;
            }
            if (iter == ".")
                continue;
            if ((child = parent.get_child_with_basename(iter)) == null) {
                child = new MockFile();
                child.basename = iter;
                associate_parent_with_child(parent, child);
            }
            parent = child;
        }

        return child ?? parent;
        // if child == null then no children created, we only went backwards
    }

    // Display name is in UTF-8, same encoding as basename
    public File get_child_for_display_name(string display_name) {
        return get_child(display_name);
    }

    // This would rename the file; return a reference to this same mock file
    public File set_display_name(string display_name, Cancellable? cancellable) {
        basename = display_name;
        return this;
    }

    /* GFileIface functions for optional capabilities that would otherwise
    return an "operation not supported" error */
    /* TODO */

    public FileEnumerator enumerate_children(string attributes,
        FileQueryInfoFlags flags, Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileInfo query_info(string attributes, FileQueryInfoFlags flags,
        Cancellable? cancellable)
    {
        var retval = new FileInfo();

        // Make sure we don't set any unwanted attributes
        var matcher = new FileAttributeMatcher(attributes);
        retval.set_attribute_mask(matcher);

        if (matcher.matches(FileAttribute.STANDARD_TYPE))
            retval.set_attribute_uint32(FileAttribute.STANDARD_TYPE,
                FileType.REGULAR);

        if (basename != null && matcher.matches(FileAttribute.STANDARD_DISPLAY_NAME))
            retval.set_attribute_string(FileAttribute.STANDARD_DISPLAY_NAME,
                basename);

        // FIXME: etags are blank for now
        if (matcher.matches(FileAttribute.ETAG_VALUE))
            retval.set_attribute_string(FileAttribute.ETAG_VALUE, "");

        return retval;
    }

    public FileInfo query_filesystem_info(string attributes,
        Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public Mount find_enclosing_mount(Cancellable? cancellable = null) throws Error {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileAttributeInfoList query_settable_attributes(Cancellable? cancellable = null)
        throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileAttributeInfoList query_writable_namespaces(Cancellable? cancellable = null)
        throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool set_attribute(string attribute, FileAttributeType type,
        void* value_p, FileQueryInfoFlags flags, Cancellable? cancellable = null)
        throws GLib.Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool set_attributes_from_info(FileInfo info,
        FileQueryInfoFlags flags, Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    // Must be declared before read() so that read() overwrites the read_fn
    // vfunc pointer in the class structure. "unowned" is a bug in this method's
    // signature, so we can't really return anything sensible here.
    [Deprecated(replacement = "read", since = "vala-0.16")]
    [NoWrapper]
    public unowned FileInputStream read_fn(Cancellable? cancellable) throws Error {
        throw new IOError.NOT_SUPPORTED("Deprecated; use read() instead.");
    }
    [CCode(vfunc_name = "read_fn")]
    public FileInputStream read(Cancellable? cancellable) {
        var istream = new MemoryInputStream.from_bytes(contents);
        return new MockFileInputStream(this, istream);
    }

    public FileOutputStream append_to(FileCreateFlags flags,
        Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileOutputStream create(FileCreateFlags flags, // ignored
        Cancellable? cancellable)
    {
        // FIXME: should we keep track of whether the mock file "exists" or not?
        var ostream = new MemoryOutputStream.resizable();
        return new MockFileOutputStream(this, ostream);
    }

    public FileOutputStream replace(string? etag, bool make_backup,
        FileCreateFlags flags, Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool @delete(Cancellable? cancellable = null) throws Error {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool trash(Cancellable? cancellable = null) throws Error {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool make_directory(Cancellable? cancellable = null) throws Error {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool make_symbolic_link(string symlink_value,
        Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool copy(File destination, FileCopyFlags flags,
        Cancellable? cancellable = null,
        FileProgressCallback? progress_callback = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public bool move(File destination, FileCopyFlags flags,
        Cancellable? cancellable = null,
        FileProgressCallback? progress_callback = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async File mount_mountable(MountMountFlags flags,
        MountOperation? mount_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool unmount_mountable(MountUnmountFlags flags,
        Cancellable? cancellable = null) throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool eject_mountable(MountUnmountFlags flags,
        Cancellable? cancellable = null) throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool mount_enclosing_volume(MountMountFlags flags,
        MountOperation? mount_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileMonitor monitor_directory(GLib.FileMonitorFlags flags,
        Cancellable? cancellable = null) throws IOError
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileMonitor monitor_file(GLib.FileMonitorFlags flags,
        Cancellable? cancellable = null) throws IOError
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileIOStream open_readwrite(Cancellable? cancellable = null)
        throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileIOStream create_readwrite(FileCreateFlags flags,
        Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public FileIOStream replace_readwrite(string? etag, bool make_backup,
        FileCreateFlags flags, Cancellable? cancellable = null) throws Error
    {
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool start_mountable(DriveStartFlags flags,
        MountOperation? start_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool stop_mountable(MountUnmountFlags flags,
        MountOperation? mount_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool unmount_mountable_with_operation(MountUnmountFlags flags,
        MountOperation? mount_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool eject_mountable_with_operation(MountUnmountFlags flags,
        MountOperation? mount_operation, Cancellable? cancellable = null)
        throws Error
    {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    public async bool poll_mountable(Cancellable? cancellable = null) throws Error {
        if (cancellable != null)
            cancellable.set_error_if_cancelled();
        throw new IOError.NOT_SUPPORTED("Not yet implemented for mock files.");
    }

    /* GFileIface functions with default implementations, e.g. async operations
    implemented in terms of running the sync operation in a different thread */
    /* TODO */
    // public override measure_disk_usage();
    // public override enumerate_children_async();
    // public override query_info_async();
    // public override query_filesystem_info_async();
    // public override find_enclosing_mount_async();
    // public override set_display_name_async();
    // public override set_attributes_async();
    // public override read_async();
    // public override append_to_async();
    // public override create_async();
    // public override replace_async();
    // public override delete_file_async();
    // public override trash_async();
    // public override make_directory_async();
    // public override copy_async();
    // public override open_readwrite_async();
    // public override create_readwrite_async();
    // public override replace_readwrite_async();
    // public override measure_disk_usage_async();

    // FIXME: what does setting this flag claim that this implementation supports?
    // FIXME: no way to override the bool field in the class structure in Vala?
    public bool supports_thread_contexts = true;

    /* Public, non-GFile API */

    public Bytes contents { set; get; default = new Bytes(new uint8[0]); }

    public string contents_utf8 {
        get {
            // get_data() can return null if length == 0
            if (contents.length == 0)
                return "";

            // (string) Bytes.get_data() doesn't add a null byte at the end!
            var bytes = new ByteArray();
            bytes.data = contents.get_data();
            bytes.append({0});
            return (string) bytes.data;
        }
        set { contents = new Bytes(value.data); }
    }
}
}  // namespace Gt