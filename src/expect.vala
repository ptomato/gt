/*
 * Copyright 2015 Philip Chimento <philip.chimento@gmail.com>
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
public class Expectation<T> {
    private T actual;
    private bool invert;
    public Expectation<T> not {
        get { invert = !invert; return this; }
    }

    public Expectation(T actual) {
        this.actual = actual;
    }

    private string print_value(T val) {
        var t = typeof(T);
        if (t == typeof(bool)) {
            return (bool) val ? "true" : "false";
        } else if (t == typeof(int)) {
            return "%d".printf((int) val);
        } else if (t == typeof(uint)) {
            return "%u".printf((uint) val);
        } else if (t == typeof(string)) {
            return "\"%s\"".printf((string) val);
        } else if (t == typeof(Regex)) {
            return "/%s/".printf(((Regex) val).get_pattern());
        } else if (t == typeof(File)) {
            if (val == null)
                return "a null file";
            return "the file %s".printf(((File) val).get_uri());
        } else if (t == typeof(Error)) {
            var error = (Error) val;
            return "%s(%d)".printf(error.domain.to_string(), error.code);
        }
        return "unknown value";
    }

    // For convenience
    private string actual_s() { return print_value(actual); }

    private unowned string print_a_type() {
        var t = typeof(T);
        if (t == typeof(string)) {
            return "a string";
        } else if (t == typeof(Object)) {
            return "an object";
        }
        return t.name();
    }

    private void fail_expected(string expectation_message) {
        Test.fail();
        Test.message("Expected " + expectation_message.replace("{actual}",
            actual_s()));
    }

    private void test(bool pass, string fail_message, string xfail_message) {
        if (!pass && !invert)
            fail_expected(fail_message);
        else if (pass && invert)
            fail_expected(xfail_message);
    }

    public void to_be_true() {
        test((bool) actual, "{actual} to be true, but it was not",
            "{actual} not to be true, but it was");
    }

    public void to_be_false() {
        test(!((bool) actual), "{actual} to be false, but it was not",
            "{actual} not to be false, but it was");
    }

    public void to_equal(T expected) {
        if (typeof(T) == typeof(File)) {
            if (expected == null && actual != null && !invert) {
                fail_expected("{actual} to equal null, but it did not");
                return;
            }
            if (expected != null && actual == null && !invert) {
                fail_expected(@"null to equal $(print_value(expected)), but it did not");
                return;
            }
            if (expected == null && actual == null) {
                if (invert)
                    fail_expected("two files not to be equal, but instead they were both null");
                return;
            }
        }

        bool equals;
        if (typeof(T) == typeof(string)) {
            // Strings would be compared by pointer unless they are cast
            equals = ((string) actual) == ((string) expected);
        } else if (typeof(T) == typeof(File)) {
            equals = ((File) expected).equal((File) actual);
        } else {
            equals = (actual == expected);
        }

        test(equals,
            @"{actual} to equal $(print_value(expected)), but it did not",
            "{actual} not to equal that value, but it did");
    }

    public void to_be_less_than(T expected) {
        // Relational operators not supported for unknown types, need to cast to
        // each integer type
        bool is_less_than;
        if (typeof(T) == typeof(int))
            is_less_than = ((int) actual) < ((int) expected);
        else if (typeof(T) == typeof(uint))
            is_less_than = ((uint) actual) < ((uint) expected);
        else
            return;

        test(is_less_than,
            @"{actual} to be less than $(print_value(expected)), but it was not",
            @"{actual} not to be less than $(print_value(expected)), but it was");
    }

    public void to_be_empty() {
        if (typeof(T) == typeof(string)) {
            var stringval = (string?) actual;
            if (stringval == null) {
                fail_expected("a string%s to be empty, but it was null".printf(invert? " not" : ""));
            } else {
                test(stringval.length == 0,
                    "{actual} to be empty, but it wasn't",
                    "a string not to be empty, but it was");
            }
        } else {
            warning("Can't expect %s to be empty", print_a_type());
        }
    }

    public void to_match(string regex_string) {
        Regex regex;
        try {
            regex = new Regex(regex_string);
        } catch (RegexError e) {
            fail_expected(@"something to match an invalid regex $regex_string: $(e.message)");
            return;
        }

        if (typeof(T) == typeof(string)) {
            var stringval = (string?) actual;

            if (stringval == null) {
                fail_expected("Expected a string%s to match %s, but it was null".printf(invert? " not" : "",
                    print_value(regex)));
                return;
            }

            test(regex.match(stringval),
                @"{actual} to match $(print_value(regex)), but it did not",
                @"{actual} not to match $(print_value(regex)), but it did");
        } else {
            warning("Can't expect %s to match %s", print_a_type(),
                print_value(regex));
        }
    }

    public void to_be_null() {
        test(actual == null, @"$(print_a_type()) to be null, but it wasn't",
            @"$(print_a_type()) not to be null, but it was");
    }

    public void to_be_a(Type type) {
        if (actual == null) {
            if (!invert)
                fail_expected(@"an object to be of type $(type.name()), but it was null");
        } else {
            var actual_type = Type.from_instance(actual);
            test(actual_type.is_a(type),
                @"an object to be of type $(type.name()), but it was of type $(actual_type.name())",
                @"an object not to be of type $(type.name()), but it was");
        }
    }

    public void to_be_clear() {
        test(actual == null, "an error to be clear, but it wasn't",
            "an error not to be clear, but it was");
    }

    public void to_match_error(Quark domain, int code) {
        test(((Error) actual).matches(domain, code),
            @"an error to match $domain($code), but it was {actual}",
            @"an error not to match $domain($code), but it did");
    }

    public void to_have_parent(T parent) {
        if (actual == null) {
            fail_expected(@"a file to have $(print_value(parent)) as its parent, but the file was null");
        } else {
            test(((File) actual).has_parent((File?) parent),
                @"{actual} to have $(print_value(parent)) as its parent, but it did not",
                @"{actual} not to have $(print_value(parent)) as its parent, but it did");
        }
    }

    public void to_have_prefix(T prefix) {
        if (actual == null) {
            fail_expected(@"a file to have $(print_value(prefix)) as a prefix, but the file was null");
        } else {
            test(((File) actual).has_prefix((File) prefix),
                @"{actual} to have $(print_value(prefix)) as a prefix, but it did not",
                @"{actual} not to have $(print_value(prefix)) as a prefix, but it did");
        }
    }

    public void to_exist() {
        if (actual == null) {
            Test.fail();
            Test.message("Expected a file to exist, but the file was null");
            return;
        }

        test(((File) actual).query_exists(null),
            "{actual} to exist, but it did not",
            "{actual} not to exist, but it did");
    }

    internal bool expect_equal_helper(T actual, string op) {
        if (op == "to_equal") {
            to_equal(actual);
            return true;
        }
        return false;
    }

    internal bool expect_null_helper(string op) {
        if (op == "to_be_null") {
            to_be_null();
            return true;
        }
        return false;
    }

    internal bool expect_compare_helper(T actual, string op) {
        switch (op) {
        case "to_be_less_than":
            to_be_less_than(actual);
            return true;
        }
        return false;
    }
}

public static Expectation<T> expect<T>(T actual) {
    return new Expectation<T>(actual);
}

// Typed API for the benefit of C

private static string adjust_expectation(string op, ref Expectation expectation) {
    if (op.has_prefix("not.")) {
        expectation = expectation.not;
        return op["not.".length:op.length];
    }
    return op;
}

public static void expect_bool(bool actual, string operation, bool expected) {
    var expectation = new Expectation<bool>(actual);
    var op = adjust_expectation(operation, ref expectation);

    if (expectation.expect_equal_helper(expected, op))
        return;
    switch (op) {
    case "to_be_true":
        expectation.to_be_true();
        return;
    case "to_be_false":
        expectation.to_be_false();
        return;
    default:
        warning(@"Unknown expectation $op for booleans");
        break;
    }
}

public static void expect_int(int actual, string operation, int expected) {
    var expectation = new Expectation<int>(actual);
    var op = adjust_expectation(operation, ref expectation);
    if (!expectation.expect_equal_helper(expected, op) &&
        !expectation.expect_compare_helper(expected, op))
        warning(@"Unknown expectation $op for integers");
}

public static void expect_uint(uint actual, string operation, uint expected) {
    var expectation = new Expectation<uint>(actual);
    var op = adjust_expectation(operation, ref expectation);
    if (!expectation.expect_equal_helper(expected, op) &&
        !expectation.expect_compare_helper(expected, op))
        warning(@"Unknown expectation $op for integers");
}

public static void expect_string(string? actual, string operation, string? expected) {
    var expectation = new Expectation<string?>(actual);
    var op = adjust_expectation(operation, ref expectation);

    if (expectation.expect_equal_helper(expected, op) ||
        expectation.expect_null_helper(op) ||
        expectation.expect_compare_helper(expected, op))
        return;
    switch (op) {
    case "to_be_empty":
        expectation.to_be_empty();
        return;
    case "to_match":
        expectation.to_match(expected);
        return;
    default:
        warning(@"Unknown expectation $op for strings");
        break;
    }
}

public static void expect_object(void *actual, string operation, Type type) {
    var expectation = new Expectation<Object?>((Object?) actual);
    var op = adjust_expectation(operation, ref expectation);

    if (expectation.expect_null_helper(op))
        return;
    switch (op) {
    case "to_be_a":
        expectation.to_be_a(type);
        return;
    default:
        warning(@"Unknown expectation $op for objects");
        break;
    }
}

public static void expect_file(File? actual, string operation, File? expected) {
    var expectation = new Expectation<File?>((File?) actual);
    var op = adjust_expectation(operation, ref expectation);

    if (expectation.expect_equal_helper(expected, op) ||
        expectation.expect_null_helper(op))
        return;
    switch (op) {
    case "to_have_parent":
        expectation.to_have_parent(expected);
        return;
    case "to_have_prefix":
        expectation.to_have_prefix(expected);
        return;
    case "to_exist":
        expectation.to_exist();
        return;
    default:
        warning(@"Unknown expectation $op for GFiles");
        break;
    }
}

public static void expect_error(Error? actual, string operation, Quark domain,
    int code)
{
    var expectation = new Expectation<Error?>(actual);
    var op = adjust_expectation(operation, ref expectation);

    switch (op) {
    case "to_be_clear":
        expectation.to_be_clear();
        return;
    case "to_match":
        expectation.to_match_error(domain, code);
        return;
    default:
        warning(@"Unknown expectation $op for errors");
        break;
    }
}

}  // namespace Gt
