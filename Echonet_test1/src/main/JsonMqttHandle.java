package main;
import org.json.simple.JSONObject;

import java.util.ArrayList;

public class JsonMqttHandle {
    public JsonMqttHandle() {
        tempJV = new JsonKeyAndValue();
        dataJson = new ArrayList<>();
    }

    private ArrayList<JsonKeyAndValue> dataJson;
    private JsonKeyAndValue tempJV;

    public ArrayList<JsonKeyAndValue> getJsonObject(JSONObject jsonObj) {
        for (Object key : jsonObj.keySet()) {

            //based on you key types
            String keyStr = key.toString();
            //Object keyValue
            Object keyValue = jsonObj.get(keyStr);
            tempJV.key = keyStr;
            tempJV.value = keyValue.toString();
            dataJson.add(tempJV);

            //for nested objects iteration if required
/*            if (keyValue instanceof JSONObject)
                getJsonObject((JSONObject)keyvalue);*/
        }
        return dataJson;
    }
    public class JsonKeyAndValue{
        public String key;
        public  String value;
    }

}
