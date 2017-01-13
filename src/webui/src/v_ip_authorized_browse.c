
/*  Dowse - embedded WebUI based on Kore.io
 *
 *  (c) Copyright 2016 Dyne.org foundation, Amsterdam
 *  Written by Nicola Rossi <nicola@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <webui.h>


int v_ip_authorized_browse(struct http_request * req,char*data) {
      char *ipaddr_type;
      char *ipaddr_value;
      char macaddr[64];
      char url_to_redirect[256];

      attributes_set_t *ptr_attrl;

      get_ip_from_request(req,&ipaddr_type,&ipaddr_value);
      if ((strcmp(ipaddr_type,"ipv4")!=0)&&(strcmp(ipaddr_type,"ipv6")!=0)) {
                kore_log(LOG_ERR,"Can retrieve IP address from request and proc file system");
                return KORE_RESULT_ERROR;
      }

      ip2mac(ipaddr_type,ipaddr_value,macaddr,ptr_attrl);

      int rv=_check_if_macaddress_is_authorized_to_browse(macaddr,ptr_attrl);

      /* Nel DB il macaddr è presente come "Enable to browse" allora il validatore deve passare
       *
       * */
      if (rv==_ENABLE_TO_BROWSE) return KORE_RESULT_OK;

      /* altrimenti deve essere rediretto ad una pagina di abilitazione o di disabilitazione */
      /*--- ... nel DB il macaddr è presente come Disabled to browse */
      if (rv==_DISABLE_TO_BROWSE) {
          /* TODO che deve fare ? */
//          sprintf(url_to_redirect,"...",macaddr);
      }

      /*--- ... nel DB il macaddr non è presente o è presente con il valore "Not enabled to browse" */
      if (rv==_NOT_ENABLE_TO_BROWSE) {
          sprintf(url_to_redirect,"/captive_client?macaddr=%s",macaddr);
      }

      /*--- ... nel DB il macaddr è presente come admin e quindi redirezionato su captive portal admin page */
      if (rv==_IP_IS_ADMIN_) {
          sprintf(url_to_redirect,"/captive_admin",macaddr);
      }

      kore_log(LOG_DEBUG," Redirecting to [%s]",url_to_redirect);
      /* from kore.io source code: Authentication types of "request" send their own HTTP responses. */
      http_response_header(req, "location", url_to_redirect);
      http_response(req, 302, NULL, 0);

      kore_free(ipaddr_type);
      kore_free(ipaddr_value);

      WEBUI_DEBUG;
      return (KORE_RESULT_ERROR);
}


int _check_if_macaddress_is_authorized_to_browse(char*macaddr,attributes_set_t*ptr_attrl){
    char query[512];
/*
    snprintf(query,sizeof(query),
            "SELECT UPPER(coalesce(authorized,'%s')) FROM found WHERE macaddr='%s'",
            __NOT_AUTHORIZED_BROWSE_STR,
            macaddr);
            */
    snprintf(query,sizeof(query),
              " SELECT "
              "   CASE "
              "     WHEN admin='yes' THEN '%s' "
              "   ELSE "
              "     UPPER(coalesce(authorized,'%s')) "
              "   END "
              " FROM found "
              " WHERE macaddr='%s'",
              __IP_IS_ADMIN_AUTH_BROWSE_STR,
              __NOT_AUTHORIZED_BROWSE_STR,
              macaddr);


    kore_log(LOG_DEBUG,"%s: query:[%s]",__where_i_am__,query);
    MYSQL_RES *result;
    MYSQL_ROW values; //  it as an array of char pointers (MYSQL_ROW),
    MYSQL_FIELD*column;
    unsigned int num_fields;
    unsigned int i;
    MYSQL *db;

    // open db connection
    db = mysql_init(NULL);

    //     Constant parameted created at compile time
    if (!mysql_real_connect(db, DB_HOST, DB_USER, DB_PASSWORD, DB_SID, 0,
            DB_SOCK_DIRECTORY, 0)) {
        show_mysql_error(db, ptr_attrl);
        db = NULL;
        return KORE_RESULT_ERROR;
    }

    // Execute the statement
    if (mysql_real_query(db, query, strlen(query))) {
        show_mysql_error(db, ptr_attrl);
        return KORE_RESULT_ERROR;
    }

    result = mysql_store_result(db);

    num_fields = mysql_num_fields(result);
    if (num_fields == 0) {
        kore_log(LOG_ERR,
                "The query [%s] has returned 0 fields. Is it correct?", query);
        return KORE_RESULT_ERROR;
    }

    kore_log(LOG_DEBUG,
            "The query [%s] has returned [%d] row with [%u] columns.", query,
            (int) mysql_affected_rows(db), num_fields);

    column = mysql_fetch_fields(result);

    int rv=_NOT_ENABLE_TO_BROWSE;
    while ((values = mysql_fetch_row(result)) != 0) {
        for (i = 0; i < num_fields; i++) {
            kore_log(LOG_DEBUG, "[%d][%s][%s]", i, column[i].name, values[i]);
        }

        if (strcmp(__ENABLE_TO_BROWSE_STR,values[0])==0){
            rv=_ENABLE_TO_BROWSE;
            break;
        };

        if (strcmp(__DISABLE_TO_BROWSE_STR,values[0])==0){
            rv=_DISABLE_TO_BROWSE;
            break;
        };
        if (strcmp(__IP_IS_ADMIN_AUTH_BROWSE_STR,values[0])==0) {
            rv=_IP_IS_ADMIN_;
            break;
        }
        kore_log(LOG_ERR,"Value not expected [%s]",values[0]);
    }
    WEBUI_DEBUG;
    mysql_free_result(result);
    mysql_close(db);

    WEBUI_DEBUG;
    return rv; /* The db is configured but it's the admin */
}
