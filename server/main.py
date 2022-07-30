#!/usr/bin/env python3
# import MySQLdb

import csv
import datetime
from flask import Flask, render_template, request
from importlib_metadata import method_cache

app = Flask(__name__)

# create table weather(timestamp datetime, temp float, press float, hum float, light float, co2 float, uva float, uvb float, uvindex int, voc float );

@app.route('/post/', methods=['GET', 'POST'])
def post():
    if request.method == 'POST':
        now = datetime.datetime.now().replace(microsecond=0)
        temp = request.json['temp']
        press = request.json['press']
        hum = request.json['hum']
        light = request.json['light']
        co2 = request.json['co2']
        uva = request.json['uva']
        uvb = request.json['uvb']
        uvindex = request.json['uvindex']
        voc = request.json['voc']
        # conn = MySQLdb.connect( user='logger', passwd='hogepass', host='localhost', db='weather_db')
        # cursor = conn.cursor()

        # conn.commit()
        # conn.close()

        with open('/home/noriokun4649/flask/data.csv', 'a') as f:
            writer = csv.writer(f)
            writer.writerow([now, temp, press, hum, light,
                            co2, uva, uvb, uvindex, voc])
        return "success"


@app.route('/get/data',methods=['GET'])
def get():
    if request.method == 'GET':
        pass


@app.route('/')
def index():
    with open('/home/noriokun4649/flask/data.csv', 'r') as f:
        reader = csv.reader(f)
        deta = [list(x) for x in zip(*[row for row in reader])]
        temp = deta[1]
        press = deta[2]
        hum = deta[3]
        light = deta[4]
        co2 = deta[5]
        uva = deta[6]
        uvb = deta[7]
        uvindex = deta[8]
        voc = deta[9]
    return render_template('index.html', main_label=deta[0], temp=temp, press=press, hum=hum, 
                            light=light, co2=co2, uva=uva, uvb=uvb, uvindex=uvindex, voc=voc)

