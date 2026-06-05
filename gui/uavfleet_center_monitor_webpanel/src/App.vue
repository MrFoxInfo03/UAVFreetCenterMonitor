<script setup lang="ts">
import { LMap, LTileLayer, LMarker } from '@vue-leaflet/vue-leaflet';
import "leaflet/dist/leaflet.css";
</script>

<template>
  <l-map
    :zoom="13"
    :center="[48.4647, 35.0462]"
    style="width: 100%; height: 100vh;"
  >
    <l-tile-layer url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"></l-tile-layer>
  </l-map>
</template>

<style scoped></style>
