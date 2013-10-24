/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

use dom::bindings::utils::{DOMString, ErrorResult};
use dom::htmlcollection::HTMLCollection;
use dom::htmlelement::HTMLElement;
use dom::node::{AbstractNode, ScriptView};

pub struct HTMLFormElement {
    htmlelement: HTMLElement
}

impl HTMLFormElement {
    pub fn AcceptCharset(&self) -> DOMString {
        None
    }

    pub fn SetAcceptCharset(&mut self, _accept_charset: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Action(&self) -> DOMString {
        None
    }

    pub fn SetAction(&mut self, _action: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Autocomplete(&self) -> DOMString {
        None
    }

    pub fn SetAutocomplete(&mut self, _autocomplete: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Enctype(&self) -> DOMString {
        None
    }

    pub fn SetEnctype(&mut self, _enctype: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Encoding(&self) -> DOMString {
        None
    }

    pub fn SetEncoding(&mut self, _encoding: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Method(&self) -> DOMString {
        None
    }

    pub fn SetMethod(&mut self, _method: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Name(&self) -> DOMString {
        None
    }

    pub fn SetName(&mut self, _name: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn NoValidate(&self) -> bool {
        false
    }

    pub fn SetNoValidate(&mut self, _no_validate: bool) -> ErrorResult {
        Ok(())
    }

    pub fn Target(&self) -> DOMString {
        None
    }

    pub fn SetTarget(&mut self, _target: &DOMString) -> ErrorResult {
        Ok(())
    }

    pub fn Elements(&self) -> @mut HTMLCollection {
        let window = self.htmlelement.element.node.owner_doc().document().window;
        HTMLCollection::new(window, ~[])
    }

    pub fn Length(&self) -> i32 {
        0
    }
    
    pub fn Submit(&self) -> ErrorResult {
        Ok(())
    }

    pub fn Reset(&self) {
    }

    pub fn CheckValidity(&self) -> bool {
        false
    }

    pub fn IndexedGetter(&self, _index: u32, _found: &mut bool) -> AbstractNode<ScriptView> {
        fail!("Not implemented.")
    }
}
