package stubs

import (
    "reflect"
)

type Domain struct {
	name      string `cpp:"std::string"  ipc:"char_string" is_key:"yes"`
	domain_id uint32 `cpp:"uint32_t" ipc:"uint32_t"`
}

//register all types for generator
var TypeRegistry = map[string]reflect.Type {
	reflect.TypeOf(Domain{}).Name() : reflect.ValueOf(Domain{}).Type(),
}
