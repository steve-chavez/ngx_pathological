.ONESHELL: # enables `cd` inside a target to work normally https://stackoverflow.com/a/30590240/4692662

NGX_MODULE_NAME=pathological_module

NGX_MODULE_LIBS=#$(shell pkg-config --libs-only-l <your_lib>)

NGX_MODULE_INCS=#$(shell pkg-config --cflags-only-I <your_lib>)

MODULE_SRC=$(wildcard src/*.c)

module_dynamic_lib=nginx/objs/$(NGX_MODULE_NAME).so

load_module_ngx_directive="load_module $(realpath .)/$(module_dynamic_lib);"

build: nginx/objs/nginx $(module_dynamic_lib)

nginx/objs/nginx: config
	cd nginx
	./auto/configure --with-compat --add-dynamic-module=..
	make

config: config.in
	sed "s/@NGX_MODULE_NAME@/$(NGX_MODULE_NAME)/g" config.in > config
	sed -i 's~@MODULE_SRC@~$(addprefix $$ngx_addon_dir/,$(MODULE_SRC))~g' config
	sed -i 's~@NGX_MODULE_LIBS@~$(NGX_MODULE_LIBS)~g' config
	sed -i 's~@NGX_MODULE_INCS@~$(NGX_MODULE_INCS)~g' config

nginx/objs/$(NGX_MODULE_NAME).so: $(MODULE_SRC)
	cd nginx
	make modules

start: nginx/objs/nginx nginx/objs/$(NGX_MODULE_NAME).so
	./nginx/objs/nginx -g $(load_module_ngx_directive) -p nix/nginx-local -e stderr

test: nginx/objs/nginx nginx/objs/$(NGX_MODULE_NAME).so
	TEST_NGINX_GLOBALS=$(load_module_ngx_directive) TEST_NGINX_BINARY=../nginx/objs/nginx prove -I. -r t

clean:
	rm -rf nginx/objs && rm config
