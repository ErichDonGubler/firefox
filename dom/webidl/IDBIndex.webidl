/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * https://dvcs.w3.org/hg/IndexedDB/raw-file/tip/Overview.html#idl-def-IDBIndexParameters
 */

dictionary IDBIndexParameters {
    boolean unique = false;
    boolean multiEntry = false;
};

interface IDBIndex {
    readonly    attribute DOMString      name;
    readonly    attribute IDBObjectStore objectStore;

    [Throws]
    readonly    attribute any            keyPath;

    readonly    attribute boolean        multiEntry;
    readonly    attribute boolean        unique;

    [Throws]
    IDBRequest openCursor (any range, optional IDBCursorDirection direction = "next");

    [Throws]
    IDBRequest openKeyCursor (any range, optional IDBCursorDirection direction = "next");

    [Throws]
    IDBRequest get (any key);

    [Throws]
    IDBRequest getKey (any key);

    [Throws]
    IDBRequest count (any key);
};

partial interface IDBIndex {
    readonly attribute DOMString storeName;

    [Throws]
    IDBRequest mozGetAll (any key, optional unsigned long limit);

    [Throws]
    IDBRequest mozGetAllKeys (any key, optional unsigned long limit);
};
